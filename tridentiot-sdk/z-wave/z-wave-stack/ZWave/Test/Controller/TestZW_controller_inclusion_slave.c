// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_controller_inclusion_slave.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include <stdint.h>
#include <ZW_controller.h>
#include <ZW_controller_network_info_storage.h>
#include <zpal_radio_utils.h>
#include <string.h>
#include <mock_control.h>
#include <ZW_CCList.h>
#include <stdlib.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}
extern void CtrlStorageSetNodeInfo_FAKE(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo);

// Number of timers initialized in a loop in ZW_controller.c
static const uint32_t TIMER_COUNT = 10;
static CommandHandler_arg_t pArgs;
static uint8_t frame_buffer[256];

CommandHandler_arg_t * command_handler_set_arguments(uint8_t * pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE * pRxOptions)
{
  pArgs.cmd = pCmd;
  printf("\npArgs.cmd: %u", *(pArgs.cmd + 1));
  pArgs.cmdLength = cmdLength;
  printf("\npArgs.cmdLength\t= %u", cmdLength);
  pArgs.rxOpt = pRxOptions;
#ifdef ZW_CONTROLLER_BRIDGE
  pArgs.multi = NULL;

#endif
  return &pArgs;
}

#define COMMAND_HANDLER_RETURN_ARG_OBJECT(pFrame, pRxOptions) \
  command_handler_set_arguments((uint8_t *)pFrame, sizeof(*pFrame), pRxOptions)

/*
 * Global variables required for the test to build.
 */
uint8_t   bNetworkWideInclusion;
uint8_t   homeID[HOMEID_LENGTH];
bool      bUseExploreAsRouteResolution;
bool      inclusionHomeIDActive;
bool      bNetworkWideInclusionReady;
uint8_t   bCurrentRoutingSpeed;
uint8_t   bMultispeedArrayByteCount;
uint8_t   abMultispeedMergeNeighbors[MAX_NODEMASK_LENGTH];
bool      protocolLockedCachedRoute;
bool      exploreRemoteInclusion;
uint8_t   crH[HOMEID_LENGTH];
uint8_t   inclusionHomeID[HOMEID_LENGTH];
uint8_t   RoutingInfoState;
uint8_t   abNeighbors;
bool      exploreInclusionModeRepeat = false;

extern void reset_static_controller_node_id(void);
extern void reset_pending_update_table(void);
extern void reset_last_used_node_id(void);
extern void reset_node_info(void);

void setUp(void)
{
  printf("\nRUN BEFORE EACH TEST!");
  mock_calls_clear();

  /*
   * Fake setup
   *
   * The tests in this module do not care HOW information is stored. As long as the information
   * can be written and read.
   */
  mock_call_use_as_fake(TO_STR(CtrlStorageGetStaticControllerNodeId));
  mock_call_use_as_fake(TO_STR(CtrlStorageSetStaticControllerNodeId));
  reset_static_controller_node_id();

  mock_call_use_as_fake(TO_STR(CtrlStorageIsNodeInPendingUpdateTable));
  mock_call_use_as_fake(TO_STR(CtrlStorageChangeNodeInPendingUpdateTable));
  reset_pending_update_table();

  mock_call_use_as_fake(TO_STR(CtrlStorageGetLastUsedNodeId));
  mock_call_use_as_fake(TO_STR(CtrlStorageSetLastUsedNodeId));
  reset_last_used_node_id();

  mock_call_use_as_fake(TO_STR(CtrlStorageSlaveNodeGet));

  mock_call_use_as_fake(TO_STR(CtrlStorageSetNodeInfo));
  mock_call_use_as_fake(TO_STR(CtrlStorageGetNodeInfo));
  mock_call_use_as_fake(TO_STR(CtrlStorageCacheNodeExist));
  reset_node_info();

  mock_call_use_as_fake(TO_STR(CtrlStorageSensorNodeGet));

  mock_call_use_as_fake(TO_STR(CtrlStorageListeningNodeGet));
}

void tearDown(void)
{
  printf("\nRUN AFTER EACH TEST!");
  mock_calls_verify();
}

/*
 * Invokes the callback passed to EnQueueSingleData().
 */
static void invoke_tx_callback(mock_t * p_mock, uint8_t status, TX_STATUS_TYPE * p_extended_status)
{
  STransmitCallback * p = (STransmitCallback *)p_mock->actual_arg[8].p;

  p->pCallback(p->Context, status, p_extended_status);
}

typedef void (*timer_callback_t)(SSwTimer* pTimer);
/*
 * Invokes the callback passed to TimerSetCallback().
 */
static void invoke_timer_callback(mock_t * p_mock)
{
  timer_callback_t tc = (timer_callback_t)p_mock->actual_arg[1].p;

  tc(p_mock->actual_arg[0].p);
}

/**
 * Expects an Assign IDs frame by setting up the mock for EnQueueSingleData().
 */
static mock_t * expect_assign_ids(uint8_t destination_node_id, uint8_t new_node_id, uint8_t * new_home_id)
{
  mock_t * p_mock;
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_CLASS_PROTOCOL;
  *(p_frame + i++) = ZWAVE_CMD_ASSIGN_IDS;
  *(p_frame + i++) = new_node_id;
  *(p_frame + i++) = *(new_home_id + 0);
  *(p_frame + i++) = *(new_home_id + 1);
  *(p_frame + i++) = *(new_home_id + 2);
  *(p_frame + i++) = *(new_home_id + 3);

  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = RF_SPEED_9_6K;
  p_mock->expect_arg[1].v = 1;
  p_mock->expect_arg[2].v = destination_node_id;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE;
  p_mock->expect_arg[6].v = 0; // No tx delay needed.
  p_mock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;
  return p_mock;
}

/**
 * Expects an NOP frame by setting up the mock for EnQueueSingleData().
 */
static mock_t * expect_nop(uint8_t destination_node_id)
{
  mock_t * p_mock;
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_NOP;

  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = RF_SPEED_9_6K;
  p_mock->expect_arg[1].v = 1;
  p_mock->expect_arg[2].v = destination_node_id;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE;
  p_mock->expect_arg[6].v = 0; // No tx delay needed.
  p_mock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;
  return p_mock;
}

typedef struct {
  uint8_t      rfSpeed;
  node_id_t    srcNodeID;
  node_id_t    destNodeID;
  uint8_t     *pData;
  uint8_t      dataLength;
  TxOptions_t  txOptions;
  uint32_t     delayedTxMs;
  uint8_t      txPower;
  STransmitCallback* pCompletedFunc;
} EnQueueSingleData_args_t;

#define ENQUEUE_SINGLE_DATA_DEFAULT_ARGUMENTS \
{ \
  RF_SPEED_9_6K, \
  1, \
  0, \
  NULL, \
  0, \
  TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE, \
  0, \
  ZPAL_RADIO_TX_POWER_DEFAULT, \
  NULL \
}

/**
 * Expects an NOP frame by setting up the mock for EnQueueSingleData().
 */
static mock_t * expect_nop_ex(EnQueueSingleData_args_t * p_args, uint32_t line_number)
{
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_NOP;

  mock_t * p_mock;
  mock_call_expect_ex(line_number, TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = p_args->rfSpeed;
  p_mock->expect_arg[1].v = p_args->srcNodeID;
  p_mock->expect_arg[2].v = p_args->destNodeID;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = p_args->txOptions;
  p_mock->expect_arg[6].v = p_args->delayedTxMs;
  p_mock->expect_arg[7].v = p_args->txPower;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;
  return p_mock;
}

#define expect_nop_v2(args) expect_nop_ex(args, __LINE__)

/**
 * Expects TimerSetCallback().
 */
mock_t * expect_timer_set_callback(void)
{
  mock_t * p_mock;
  mock_call_expect(TO_STR(TimerSetCallback), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_ANY;
  p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL; // Callback function
  return p_mock;
}

#define expect_find_nodes_in_range(destination_node_id, bitmask, bitmask_length, wakeup_time, speed)\
                                   expect_find_nodes_in_range_v2(destination_node_id,  bitmask, bitmask_length,\
                                   wakeup_time, speed, (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE))
/**
 * Expects an Find Nodes In Range frame by setting up the mock for EnQueueSingleData().
 */
static mock_t * expect_find_nodes_in_range_v2(uint8_t destination_node_id,
                                           uint8_t * bitmask,
                                           uint8_t bitmask_length,
                                           uint8_t wakeup_time,
                                           uint8_t speed,
                                           uint8_t tx_options)
{
  mock_t * p_mock;
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_CLASS_PROTOCOL;
  *(p_frame + i++) = ZWAVE_CMD_FIND_NODES_IN_RANGE;
  uint8_t speed_present = 0; // TODO: Hardcoded because it seems that production code doesn't set it.
  *(p_frame + i++) = (speed_present & 0x80) | (bitmask_length & 0x1F);
  for (uint8_t j = 0; j < bitmask_length; j++)
  {
    *(p_frame + i++) = bitmask[j];
  }
  *(p_frame + i++) = wakeup_time;
  *(p_frame + i++) = speed & 0x07;

  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = 0;
  p_mock->expect_arg[1].v = 1;
  p_mock->expect_arg[2].v = destination_node_id;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = tx_options;
  p_mock->expect_arg[6].v = 0; // No tx delay needed.
  p_mock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;
  return p_mock;
}


/**
 * Expects an Get Nodes In Range frame by setting up the mock for EnQueueSingleData().
 */
static mock_t * expect_get_nodes_in_range(uint8_t destination_node_id, uint8_t tx_options)
{
  mock_t * p_mock;
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_CLASS_PROTOCOL;
  *(p_frame + i++) = ZWAVE_CMD_GET_NODES_IN_RANGE;

  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = 0;
  p_mock->expect_arg[1].v = 1;
  p_mock->expect_arg[2].v = destination_node_id;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = tx_options;
  p_mock->expect_arg[6].v = 0; // No tx delay needed.
  p_mock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;
  return p_mock;
}

/**
 * Expects ZwTimerRegister().
 */
mock_t * expect_zw_timer_register(void)
{
  mock_t * p_mock;
  mock_call_expect(TO_STR(ZwTimerRegister), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_ANY;
  p_mock->compare_rule_arg[1] = COMPARE_ANY;
  p_mock->compare_rule_arg[2] = COMPARE_ANY;
  p_mock->return_code.v = true;
  return p_mock;
}

static void learn_mode_callback(LEARN_INFO_T * pLearnInfo) {
  printf("\n################################");
  printf("\n# Learn mode callback invoked! #");
  if (ADD_NODE_STATUS_FAILED == pLearnInfo->bStatus)
  {
    printf("\n# Learn mode FAILED!           #");
  }
  if (ADD_NODE_STATUS_DONE == pLearnInfo->bStatus)
  {
    printf("\n# Learn mode DONE!             #");
  }
  printf("\n# Status: %02d                   #", pLearnInfo->bStatus);
  printf("\n# Source Node ID: %02d           #", pLearnInfo->bSource);
  printf("\n################################");
}

static void NeighborUpdate_callback(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus) {
  (void)Context;
  (void)extendedTxStatus;
  printf("\n################################");
  printf("\n# ZW_RequestNodeTypeNeighborUpdate! #");
  if (REQUEST_NEIGHBOR_UPDATE_FAILED == txStatus)
  {
    printf("\n# ZW_RequestNodeTypeNeighborUpdate FAILED!           #");
  }
  if (REQUEST_NEIGHBOR_UPDATE_DONE == txStatus)
  {
    printf("\n# ZW_RequestNodeTypeNeighborUpdate DONE!             #");
  }
  printf("\n################################");
}

static void set_suc_callback(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus) {
  (void)Context;
  (void)extendedTxStatus;
  printf("\n################################");
  printf("\n# Set SUC callback invoked!    #");
  printf("\n# Status: %02d                   #", txStatus);
  printf("\n################################");
}

/*
 * Verifies the frame flow when non-securely including a slave.
 *
 * Does not verify Transfer Presentation frames.
 *
 * Host                  Controller                 Slave
 *  |-----enter-add-mode----->|                       |
 *  |                         |<---------NIF----------|
 *  |                         |------Assign IDs------>|
 *  |                         |<---------ACK----------| (callback)
 *  |                         |----------NOP--------->|
 *  |                         |<---------ACK----------| (callback)
 *  |                         |--Find nodes in range->|
 *  |                         |<---------ACK----------| (callback)
 *  |                         |<------NOP Power-------| (not part of test)
 *  |                         |----------ACK--------->| (not part of test)
 *  |                         |<---Command Complete---|
 *  |                         |----------ACK--------->| (not part of test)
 *  |                         |--Get nodes in range-->|
 *  |                         |<---------ACK----------| (callback)
 *  |                         |<---Node Range Info----|
 *  |                         |----------ACK--------->| (not part of test)
 *  |------stop-add-mode----->|
 */
void test_inclusion_of_slave(void) {
//  TEST_IGNORE();
  mock_t * p_mock;

  // The node ID is read from NVM, but this happens in ZW_main and not in the controller module.
  // Hence, we must set the ID here because it's used in frames sent from the controller.
  g_nodeID = 1;

  // Stub setup
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(UpdateMostUsedNodes));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(llGetCurrentHeaderFormat));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(RangeInfoNeeded));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLineExile));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLinePurge));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(MergeNoneListeningNodes));
  mock_call_use_as_stub(TO_STR(RoutingInfoReceived));
  mock_call_use_as_stub(TO_STR(ClearPendingDiscovery));
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer (via add node to network)
  expect_zw_timer_register(); // Replication timer (via add node to network)

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);
  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  /*
   * Put the controller into add mode - invoked by controller host
   *
   * Will invoke learn_mode_callback() once.
   */
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer
  expect_timer_set_callback(); // PresentationTimer
  ZW_AddNodeToNetwork(ADD_NODE_ANY, learn_mode_callback);

  /*
   * Generate a fake slave NIF
   */
  NODEINFO_SLAVE_FRAME f = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_NODE_INFO,
                            .capability = ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                            .security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SLAVE_ROUTING | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                            .reserved = ZWAVE_NODEINFO_BAUD_100K,
                            .nodeType = {.generic = GENERIC_TYPE_SWITCH_BINARY, .specific = SPECIFIC_TYPE_NOT_USED}
  };
  for (uint8_t i = 0; i < NODEPARM_MAX; i++) {
    f.nodeInfo[i] = i;
  }

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;

  // Destination node ID is zero because the slave is not included yet
  mock_t * p_mock_Assign_IDs = expect_assign_ids(0, 2, home_id.as_array);

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&f, &rxOptions));

  mock_t * p_mock_nop = expect_nop(2);

  invoke_tx_callback(p_mock_Assign_IDs, TRANSMIT_COMPLETE_OK, NULL);

  uint8_t nodemask[] = {0x01};
  mock_t * p_mock_find_nodes_in_range = expect_find_nodes_in_range(2, nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K);

  invoke_tx_callback(p_mock_nop, TRANSMIT_COMPLETE_OK, NULL);

  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * At this point, the slave will send a nop power to the controller, but we do not care about
   * that since it will be ack'ed at a lower level.
   */

  mock_t * p_mock_get_nodes_in_range = expect_get_nodes_in_range(2, (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE));

  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   */
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  COMMAND_COMPLETE_FRAME command_complete_frame = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_CMD_COMPLETE,
                            .seqNo = 0
  };

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);
  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  mock_t * p_mock_LearnInfoTimer = expect_timer_set_callback();


  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  const uint8_t range_info_frame[] = {
                                ZWAVE_CMD_CLASS_PROTOCOL,
                                ZWAVE_CMD_RANGE_INFO,
                                1,
                                1,
                                0
  };

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));

  invoke_timer_callback(p_mock_LearnInfoTimer);

  ZW_AddNodeToNetwork(ADD_NODE_STOP, learn_mode_callback);

}

/*
 * Verifies the frame flow where
 * 1. a secondary controller (B) is included,
 * 2. the secondary controller (B) is set to be SUC (without SIS),
 * 3. a slave is included,
 * 4. the secondary controller is informed that the slave was included,
 * 5. the slave is excluded,
 * 6. the secondary controller is informed that the slave was excluded, and
 * 7. the secondary controller is excluded.
 *
 * Host                   Controller A        Controller B         Slave
 *  |-----enter-add-mode----->|                   |                  |
 *  |                         |<-------NIF--------|                  |
 *  |                         |----Assign IDs---->|                  |
 *  |                         |<-------ACK--------|                  | (callback)
 *  |                         |--------NOP------->|                  |
 *  |                         |<-------ACK--------|                  | (callback)
 *  |-------Set-SUC-(B)------>|------Set-SUC----->|                  |
 *  |                         |<-------ACK--------|                  | (callback)
 *  |                         |<---Set SUC ACK----|                  |
 *  |                         |--------ACK------->|                  | (not part of test)
 *  |------stop-add-mode----->|                   |                  |
 *  |-----enter-add-mode----->|                   |                  |
 *  |                         |<-----------------NIF-----------------|
 *  |                         |---------------Assign IDs------------>|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |                         |------------------NOP---------------->|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |                         |---------Find nodes in range--------->|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |                         |<--------------NOP Power--------------| (not part of test)
 *  |                         |------------------ACK---------------->| (not part of test)
 *  |                         |                   |<---NOP Power-----| (not part of test)
 *  |                                             |-------ACK------->| (not part of test)
 *  |                         |<-----------Command Complete----------|
 *  |                         |------------------ACK---------------->| (not part of test)
 *  |                         |----------Get nodes in range--------->|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |                         |<-----------Node Range Info-----------|
 *  |                         |------------------ACK---------------->| (not part of test)
 *  |                         |--------Assign SUC Return Route------>|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |------stop-add-mode----->|                   |                  |
 *  |                         |---New-node-reg--->|
 *  |                         |<-------ACK--------| (callback)
 *  |                         |---New-range-reg-->|
 *  |                         |<-------ACK--------| (callback)
 *
 *  At this point inclusion is done. Then follows the flow of exclusion:
 *
 *  |----enter-remove-mode----|                   |                  |
 *  |                         |<-----------------NIF-----------------|
 *  |                         |---------------Assign IDs------------>|
 *  |                         |<-----------------ACK-----------------| (callback)
 *  |                         |------------------NOP---------------->|
 *  |                         |<---------------- NACK----------------| (callback)
 *  |                         |---New-node-reg--->|
 *  |                         |<-------ACK--------| (callback)
 *  |----stop-remove-mode---->|                   |                  |
 *  |----enter-remove-mode----|                   |                  |
 *  |                         |<-------NIF--------|                  |
 *  |                         |----Assign IDs---->|                  |
 *  |                         |<-------ACK--------|                  | (callback)
 *  |                         |--------NOP------->|                  |
 *  |                         |<-------NACK-------|                  | (callback)
 *  |----stop-remove-mode---->|                   |                  |
 *
 */
void test_inclusion_of_controller_and_slave(void)
{
//  TEST_IGNORE();

  mock_t * p_mock;

  // The node ID is read from NVM, but this happens in ZW_main and not in the controller module.
  // Hence, we must set the ID here because it's used in frames sent from the controller.
  g_nodeID = 1;

  // Stub setup
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(UpdateMostUsedNodes));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(llGetCurrentHeaderFormat));
  mock_call_use_as_stub(TO_STR(RangeInfoNeeded));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLineExile));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLinePurge));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(MergeNoneListeningNodes));
  mock_call_use_as_stub(TO_STR(RoutingInfoReceived));
  mock_call_use_as_stub(TO_STR(ClearPendingDiscovery));
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
  mock_call_use_as_stub(TO_STR(ProtocolInterfacePassToAppNodeUpdate));
  mock_call_use_as_stub(TO_STR(ZCB_HasNodeANeighbour));
  mock_call_use_as_stub(TO_STR(ZW_AreNodesNeighbours));
  mock_call_use_as_stub(TO_STR(RoutingAnalysisStop));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_use_as_stub(TO_STR(ZW_SetRoutingInfo));
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */
  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  SAppNodeInfo_t ControllerNodeInfo;
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  typedef union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id_t;
  home_id_t home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  /* ***********************************************************************************************
   * Include controller B
   * ***********************************************************************************************
   */

  /*
   * Put the controller into add mode - invoked by controller host
   *
   * Will invoke learn_mode_callback() once.
   */
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer
  expect_timer_set_callback(); // PresentationTimer
  ZW_AddNodeToNetwork(ADD_NODE_ANY, learn_mode_callback);

  /*
   * Generate a fake controller NIF
   */
  NODEINFO_FRAME f = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_NODE_INFO,
                            .capability = ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                            .security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_CONTROLLER_NODE | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                            .reserved = ZWAVE_NODEINFO_BAUD_100K,
                            .nodeType = {.basic = BASIC_TYPE_STATIC_CONTROLLER, .generic = GENERIC_TYPE_STATIC_CONTROLLER, .specific = SPECIFIC_TYPE_PC_CONTROLLER}
  };
  for (uint8_t i = 0; i < NODEPARM_MAX; i++) {
    f.nodeInfo[i] = i;
  }

  const node_id_t CONTROLLER_INCLUDED_NODE_ID = 1; // Every controller starts out with node ID 1.

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = CONTROLLER_INCLUDED_NODE_ID;
  // A controller being included will have it's own home ID which is different from our home ID.
  rxOptions.rxStatus = RECEIVE_STATUS_FOREIGN_HOMEID;

  // Destination node ID is 1 because destination is a controller.
  mock_t * p_mock_Assign_IDs = expect_assign_ids(1, 2, home_id.as_array);

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&f, &rxOptions));

  mock_t * p_mock_nop = expect_nop(2);

  invoke_tx_callback(p_mock_Assign_IDs, TRANSMIT_COMPLETE_OK, NULL);

  uint8_t nodemask_ctrl[] = {0x01};
  mock_t * p_mock_EnQueueSingleData_find_nodes_in_range = expect_find_nodes_in_range(2, nodemask_ctrl, sizeof(nodemask_ctrl), 0, ZWAVE_FIND_NODES_SPEED_100K);

  invoke_tx_callback(p_mock_nop, TRANSMIT_COMPLETE_OK, NULL);

  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * At this point, the slave will send a nop power to the controller, but we do not care about
   * that since it will be ack'ed at a lower level.
   */
  mock_t * p_mock_EnQueueSingleData_get_nodes_in_range = expect_get_nodes_in_range(2, (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE));

  /*
   *
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   *
   */
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  COMMAND_COMPLETE_FRAME command_complete_frame = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_CMD_COMPLETE,
                            .seqNo = 0
  };

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  mock_t * p_mock_LearnInfoTimer = expect_timer_set_callback(); // LearnInfoTimer

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  const uint8_t range_info_frame[] = {
                                ZWAVE_CMD_CLASS_PROTOCOL,
                                ZWAVE_CMD_RANGE_INFO,
                                1,
                                1,
                                0
  };

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));

  // Expect Transfer Node Info frame about controller A (1) being sent to controller B (2)
  const TRANSFER_NODE_INFO_FRAME transfer_node_info_frame_ctrl_1 = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_TRANSFER_NODE_INFO,
                                       .seqNo = 1,
                                       .nodeID = 1,
                                       .nodeInfo = {
                                                    .capability = ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                                                    .security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_CONTROLLER_NODE | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                                                    .reserved = ZWAVE_NODEINFO_BAUD_100K | ZWAVE_NODEINFO_BAUD_100KLR,
                                                    .nodeType = {
                                                                 .basic = BASIC_TYPE_STATIC_CONTROLLER,
                                                                 .generic = GENERIC_TYPE_STATIC_CONTROLLER,
                                                                 .specific = SPECIFIC_TYPE_PC_CONTROLLER
                                                    }
                                       }
  };

  mock_t * p_mock_EnQueueSingleData_transfer_node_info_frame;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_node_info_frame);
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[3].p = (uint8_t *)&transfer_node_info_frame_ctrl_1;
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[4].v = sizeof(TRANSFER_NODE_INFO_FRAME);
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_node_info_frame->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_node_info_frame->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_node_info_frame->return_code.v = true;

  invoke_timer_callback(p_mock_LearnInfoTimer);

  expect_timer_set_callback(); // replicationTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_transfer_node_info_frame, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   *
   */

  // Expect Transfer Node Info frame about controller B (2) being sent to controller B (2)
  const TRANSFER_NODE_INFO_FRAME transfer_node_info_frame_ctrl_2 = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_TRANSFER_NODE_INFO,
                                       .seqNo = 2,
                                       .nodeID = 2,
                                       .nodeInfo = {
                                                    .capability = ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                                                    .security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_CONTROLLER_NODE | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                                                    .reserved = ZWAVE_NODEINFO_BAUD_100K,
                                                    .nodeType = {
                                                                 .basic = BASIC_TYPE_STATIC_CONTROLLER,
                                                                 .generic = GENERIC_TYPE_STATIC_CONTROLLER,
                                                                 .specific = SPECIFIC_TYPE_PC_CONTROLLER
                                                    }
                                       }
  };

  mock_t * p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2);
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[3].p = (uint8_t *)&transfer_node_info_frame_ctrl_2;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[4].v = sizeof(TRANSFER_NODE_INFO_FRAME);
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2->return_code.v = true;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  command_complete_frame.seqNo = 1;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  expect_timer_set_callback(); // replicationTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_transfer_node_info_frame_ctrl_2, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   *
   */
  mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
  p_mock->expect_arg[0].v = 1;
  p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  uint8_t nodemask[MAX_NODEMASK_LENGTH];
  memset(nodemask, 0, MAX_NODEMASK_LENGTH);
  nodemask[0] = 0x02;
  p_mock->output_arg[1].p = nodemask;
  p_mock->expect_arg[2].v = 0;

  // Expect Transfer Range Info frame about controller A (1) being sent to controller B (2)
  TRANSFER_RANGE_INFO_FRAME transfer_range_info_frame = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_TRANSFER_RANGE_INFO,
                                       .seqNo = 3,
                                       .nodeID = 1,
                                       .numMaskBytes = 29
  };
  memset((uint8_t *)transfer_range_info_frame.maskBytes, 0, MAX_NODEMASK_LENGTH);
  transfer_range_info_frame.maskBytes[0] = 0x02; // Controller A can see Controller B with node ID 2, hence bit 1

  mock_t * p_mock_EnQueueSingleData_transfer_range_info_frame;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_range_info_frame);
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[3].p = (uint8_t *)&transfer_range_info_frame;
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[4].v = sizeof(TRANSFER_RANGE_INFO_FRAME);
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_range_info_frame->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_range_info_frame->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_range_info_frame->return_code.v = true;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  command_complete_frame.seqNo = 2;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  expect_timer_set_callback(); // replicationTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_transfer_range_info_frame, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   *
   */
  mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
  p_mock->expect_arg[0].v = 2;
  p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  memset(nodemask, 0, MAX_NODEMASK_LENGTH);
  nodemask[0] = 0x01;
  p_mock->output_arg[1].p = nodemask;
  p_mock->expect_arg[2].v = 0;

  // Expect Transfer Range Info frame about controller A (1) being sent to controller B (2)
  TRANSFER_RANGE_INFO_FRAME transfer_range_info_frame_ctrl_2 = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_TRANSFER_RANGE_INFO,
                                       .seqNo = 4,
                                       .nodeID = 2,
                                       .numMaskBytes = 29
  };
  memset((uint8_t *)transfer_range_info_frame_ctrl_2.maskBytes, 0, MAX_NODEMASK_LENGTH);
  transfer_range_info_frame_ctrl_2.maskBytes[0] = 0x01; // Controller B can see Controller A with node ID 1, hence bit 0

  mock_t * p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2);
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[3].p = (uint8_t *)&transfer_range_info_frame_ctrl_2;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[4].v = sizeof(TRANSFER_RANGE_INFO_FRAME);
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2->return_code.v = true;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  command_complete_frame.seqNo = 3;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  expect_timer_set_callback(); // replicationTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_transfer_range_info_frame_ctrl_2, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   *
   */
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2;

  command_complete_frame.seqNo = 4;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  /*
   *
   * The last part of including a controller is initiated by the host.
   *
   * The host invokes ZW_AddNodeToNetwork() with ADD_NODE_STOP.
   *
   * This leads to transmitting a Send SUC ID and Transfer End.
   *
   */
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  // Expect Send SUC ID frame being sent to controller B (2)
  SUC_NODE_ID_FRAME suc_node_id_frame = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_SUC_NODE_ID,
                                       .nodeID = 0,
                                       .SUCcapabilities = 0
  };

  mock_t * p_mock_EnQueueSingleData_suc_node_id_frame;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_suc_node_id_frame);
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[3].p = (uint8_t *)&suc_node_id_frame;
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[4].v = sizeof(SUC_NODE_ID_FRAME);
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_suc_node_id_frame->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_suc_node_id_frame->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_suc_node_id_frame->return_code.v = true;

  ZW_AddNodeToNetwork(ADD_NODE_STOP, learn_mode_callback);

  // Expect Transfer End frame being sent to controller B (2)
  TRANSFER_END_FRAME transfer_end_frame = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_TRANSFER_END,
                                       .status = ZWAVE_TRANSFER_OK // TODO: Should be value 0x02 according to SDS14641 ?!
  };

  mock_t * p_mock_EnQueueSingleData_transfer_end_frame;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_end_frame);
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[3].p = (uint8_t *)&transfer_end_frame;
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[4].v = sizeof(TRANSFER_END_FRAME);
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_end_frame->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_end_frame->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_end_frame->return_code.v = true;

  invoke_tx_callback(p_mock_EnQueueSingleData_suc_node_id_frame, TRANSMIT_COMPLETE_OK, NULL);

  invoke_tx_callback(p_mock_EnQueueSingleData_transfer_end_frame, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * Setup mock expectations and set controller B to SUC
   */

  // Expect NOP frame being sent to the slave
  const SET_SUC_FRAME set_suc_frame = {
                                       .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                       .cmd = ZWAVE_CMD_SET_SUC,
                                       .state = true,
                                       .SUCcapabilities = 0
  };

  mock_t * p_mock_EnQueueSingleData_set_suc_frame;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_set_suc_frame);
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[2].v = 2; // Controller assigned node ID
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[3].p = (uint8_t *)&set_suc_frame;
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[4].v = sizeof(SET_SUC_FRAME);
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_set_suc_frame->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_set_suc_frame->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_set_suc_frame->return_code.v = true;

  STransmitCallback ZW_SetSUCNodeID_callback = { .pCallback = set_suc_callback, .Context = NULL };
  ZW_SetSUCNodeID(2, true, false, 0, &ZW_SetSUCNodeID_callback);

  /*
   * Fetch callback from Set SUC, setup mocks and invoke callback.
   */

  expect_timer_set_callback();

  invoke_tx_callback(p_mock_EnQueueSingleData_set_suc_frame, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * Set mock expectations and call command handler with Set SUC ACK command
   */

  SET_SUC_ACK_FRAME set_suc_ack_frame = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_SET_SUC_ACK,
                            .result = 0x01,
                            .SUCcapabilities = 0x00
  };

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 2; // Controller B has gotten ID 2

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&set_suc_ack_frame, &rxOptions));

  /* ***********************************************************************************************
   * Include slave
   * ***********************************************************************************************
   */

  const uint8_t NODE_ID_SLAVE = 3;

  printf("\n***********************");
  printf("\n* Including slave ... *");
  printf("\n***********************");

    /*
     * Put the controller into add mode - invoked by controller host
     *
     * Will invoke learn_mode_callback() once.
     */
    expect_zw_timer_register(); // Presentation timer
    expect_zw_timer_register(); // Replication timer
    expect_timer_set_callback(); // &PresentationTimer
    ZW_AddNodeToNetwork(ADD_NODE_ANY, learn_mode_callback);

  /*
   * Generate a fake slave NIF
   */
  NODEINFO_SLAVE_FRAME nif_slave = {
                            .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                            .cmd = ZWAVE_CMD_NODE_INFO,
                            .capability = ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                            .security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SLAVE_ROUTING | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                            .reserved = ZWAVE_NODEINFO_BAUD_100K,
                            .nodeType = {.generic = GENERIC_TYPE_SWITCH_BINARY, .specific = SPECIFIC_TYPE_NOT_USED}
  };

  uint8_t nrOfCmdclasses = 15;
  // Generate 15 commandclasses
  for (uint8_t i = 0; i < nrOfCmdclasses; i++) {
    nif_slave.nodeInfo[i] = i + 0x20;
  }

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;

  mock_t * p_mock_Assign_IDs_slave = expect_assign_ids(0, NODE_ID_SLAVE, home_id.as_array);

//  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&nif_slave, &rxOptions));
  CommandHandler(command_handler_set_arguments((uint8_t *)&nif_slave, sizeof(nif_slave) - sizeof(nif_slave.nodeInfo) + nrOfCmdclasses, &rxOptions));

  mock_t * p_mock_nop_slave = expect_nop(3);

  invoke_tx_callback(p_mock_Assign_IDs_slave, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * Setup mock expectations and invoke the callback to signal an ack to the NOP frame sent from the slave.
   */

  mock_call_expect(TO_STR(TimerIsActive), &p_mock); // TimerHandler
  p_mock->compare_rule_arg[0] = COMPARE_ANY;
  p_mock->return_code.v = false; // Must be inactive so that the pending update timer is started.

  expect_timer_set_callback(); // TimerHandler

  mock_t * p_mock_AssignTxCompleteDelayTimer = expect_timer_set_callback();

  invoke_tx_callback(p_mock_nop_slave, TRANSMIT_COMPLETE_OK, NULL);

  mock_call_expect(TO_STR(TimerIsActive), &p_mock); // TimerHandler
  p_mock->compare_rule_arg[0] = COMPARE_ANY;
  p_mock->return_code.v = true; // Must be active because TimerHandler was started earlier.

  uint8_t nodemask_slave[] = {0x01 | 0x02};
  mock_t * p_mock_EnQueueSingleData_find_nodes_in_range_slave = expect_find_nodes_in_range(3, nodemask_slave, sizeof(nodemask_slave), 0, ZWAVE_FIND_NODES_SPEED_100K);

  invoke_timer_callback(p_mock_AssignTxCompleteDelayTimer);

  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_tx_callback(p_mock_EnQueueSingleData_find_nodes_in_range_slave, TRANSMIT_COMPLETE_OK, NULL);

  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
   */
  expect_get_nodes_in_range(3, (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE)); // Ignore the returned mock because the following Command Complete will trigger the next step.

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 3;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));

  // Expect an Assign SUC Return Route frame being sent to the slave
  uint8_t assign_suc_return_route_frame[] = {
                         ZWAVE_CMD_CLASS_PROTOCOL,
                         ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE,
                         2,
                         0,
                         0x20
  };

  mock_t * p_mock_EnQueueSingleData_assign_suc_return_route;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_assign_suc_return_route);
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[2].v = 3;  // Slave assigned node ID
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[3].p = (uint8_t *)&assign_suc_return_route_frame;
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[4].v = sizeof(assign_suc_return_route_frame);
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_assign_suc_return_route->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_assign_suc_return_route->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_assign_suc_return_route->return_code.v = true;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 3;

  const uint8_t range_info_frame_from_slave[] = {
                                ZWAVE_CMD_CLASS_PROTOCOL,
                                ZWAVE_CMD_RANGE_INFO,
                                1,
                                0x01 | 0x02,
                                0
  };

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame_from_slave, &rxOptions));
//#endif

  /*
   * Fetch callback passed to EnQueueSingleData() when transmitting Assign SUC Return Route.
   */

  mock_t * p_mock_LearnInfoTimer_slave = expect_timer_set_callback();

  invoke_tx_callback(p_mock_EnQueueSingleData_assign_suc_return_route, TRANSMIT_COMPLETE_OK, NULL);
  /*
   * Fetch callback passed to TimerSetCallback() on callback from Assign SUC Return Route.
   */
  mock_call_expect(TO_STR(TimerIsActive), &p_mock); // TimerHandler
  p_mock->compare_rule_arg[0] = COMPARE_ANY;
  p_mock->return_code.v = true;

  invoke_timer_callback(p_mock_LearnInfoTimer_slave);

  /*
   * Stop add mode
   */

  // Expect a New Node Registered frame being sent to controller B
  const uint8_t new_node_registered_frame[] = {
                                                               ZWAVE_CMD_CLASS_PROTOCOL,
                                                               ZWAVE_CMD_NEW_NODE_REGISTERED,
                                                               3,
                                                               ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK,
                                                               ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SLAVE_ROUTING | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE,
                                                               ZWAVE_NODEINFO_BAUD_100K,
                                                               GENERIC_TYPE_SWITCH_BINARY,
                                                               SPECIFIC_TYPE_NOT_USED,
                                                               0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e
  };

  mock_t * p_mock_EnQueueSingleData_new_node_registered;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_new_node_registered);
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[2].v = 2;  // Controller B
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[3].p = (uint8_t *)&new_node_registered_frame;
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[4].v = sizeof(new_node_registered_frame);
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_new_node_registered->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_new_node_registered->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_new_node_registered->return_code.v = true;

  ZW_AddNodeToNetwork(ADD_NODE_STOP, learn_mode_callback);

  mock_t * p_mock_UpdateTimeoutTimer = expect_timer_set_callback();

  invoke_tx_callback(p_mock_EnQueueSingleData_new_node_registered, TRANSMIT_COMPLETE_OK, NULL);

  mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
  p_mock->expect_arg[0].v = 3; // Slave node ID
  p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  uint8_t mask[MAX_NODEMASK_LENGTH];
  for (uint8_t i = 0; i < MAX_NODEMASK_LENGTH; i++)
  {
    mask[i] = i;
  }
  p_mock->output_arg[1].p = mask;
  p_mock->expect_arg[2].v = 0; // No options

  // Expect a New Range Registered frame being sent to controller B
  NEW_RANGE_REGISTERED_FRAME new_range_registered_frame = {
                                                               .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
                                                               .cmd = ZWAVE_CMD_NEW_RANGE_REGISTERED,
                                                               .nodeID = 3,
                                                               .numMaskBytes = MAX_NODEMASK_LENGTH
  };
  memcpy((uint8_t *)&new_range_registered_frame.maskBytes, mask, MAX_NODEMASK_LENGTH);

  mock_t * p_mock_EnQueueSingleData_new_range_registered;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_new_range_registered);
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[2].v = 2;  // Controller B
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[3].p = (uint8_t *)&new_range_registered_frame;
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[4].v = sizeof(NEW_RANGE_REGISTERED_FRAME);
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_new_range_registered->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_new_range_registered->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_new_range_registered->return_code.v = true;

  invoke_timer_callback(p_mock_UpdateTimeoutTimer);

  mock_t * p_mock_UpdateTimeoutTimer_ZCB_DelayPendingUpdate = expect_timer_set_callback();

  invoke_tx_callback(p_mock_EnQueueSingleData_new_range_registered, TRANSMIT_COMPLETE_OK, NULL);

  invoke_timer_callback(p_mock_UpdateTimeoutTimer_ZCB_DelayPendingUpdate);


  /* *********************************************************************************************
   * Removal of slave
   * *********************************************************************************************
   */

  printf("\n***********************");
  printf("\n* Removing slave ... *");
  printf("\n***********************");

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  expect_timer_set_callback(); // Presentation timer

  ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, &learn_mode_callback);

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = NODE_ID_SLAVE;

  home_id_t new_home_id;
  new_home_id.as_uint32 = 0x00000000;

  mock_t * p_mock_Assign_IDs_slave_exclude = expect_assign_ids(NODE_ID_SLAVE, 0, new_home_id.as_array);

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&nif_slave, &rxOptions));

  EnQueueSingleData_args_t args = ENQUEUE_SINGLE_DATA_DEFAULT_ARGUMENTS;
  args.rfSpeed = 0;
  args.destNodeID = NODE_ID_SLAVE;
  mock_t * p_mock_nop_slave_exclude_1 = expect_nop_v2(&args);

  invoke_tx_callback(p_mock_Assign_IDs_slave_exclude, TRANSMIT_COMPLETE_OK, NULL);

  mock_call_use_as_stub(TO_STR(DeleteMostUsed));
  mock_call_use_as_stub(TO_STR(RoutingRemoveNonRepeater));
  mock_call_use_as_stub(TO_STR(ResetRouting));

  const uint8_t new_node_registered_frame_delete[] = {
                                                               ZWAVE_CMD_CLASS_PROTOCOL,
                                                               ZWAVE_CMD_NEW_NODE_REGISTERED,
                                                               NODE_ID_SLAVE,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0 // TODO: Should not be there
  };

  mock_t * p_mock_EnQueueSingleData_new_node_registered_delete;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_new_node_registered_delete);
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[2].v = 2;  // Controller B
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[3].p = (uint8_t *)&new_node_registered_frame_delete;
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[4].v = sizeof(new_node_registered_frame_delete);
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_new_node_registered_delete->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_new_node_registered_delete->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_new_node_registered_delete->return_code.v = true;

  invoke_tx_callback(p_mock_nop_slave_exclude_1, TRANSMIT_COMPLETE_NO_ACK, NULL);

  invoke_tx_callback(p_mock_EnQueueSingleData_new_node_registered_delete, TRANSMIT_COMPLETE_OK, NULL);

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, &learn_mode_callback);

  /* *********************************************************************************************
   * Removal of controller B
   * *********************************************************************************************
   */

  printf("\n*****************************");
  printf("\n* Removing controller B ... *");
  printf("\n*****************************");

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  expect_timer_set_callback(); // Presentation timer

  ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, &learn_mode_callback);

  const node_id_t NODE_ID_CONTROLLER_B = 2;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = NODE_ID_CONTROLLER_B;

  new_home_id.as_uint32 = 0x00000000;

  mock_t * p_mock_Assign_IDs_controller_exclude = expect_assign_ids(NODE_ID_CONTROLLER_B, 0, new_home_id.as_array);

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&f, &rxOptions));

  EnQueueSingleData_args_t nop_args = ENQUEUE_SINGLE_DATA_DEFAULT_ARGUMENTS;
  nop_args.rfSpeed = 0;
  nop_args.destNodeID = NODE_ID_CONTROLLER_B;
  mock_t * p_mock_nop_controller_exclude_1 = expect_nop_v2(&nop_args);

  invoke_tx_callback(p_mock_Assign_IDs_controller_exclude, TRANSMIT_COMPLETE_OK, NULL);

  mock_t * p_mock_EnQueueSingleData_transfer_end_frame_remove;
  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock_EnQueueSingleData_transfer_end_frame_remove);
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[0].v = 0;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[1].v = 1;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[2].v = 0; // Controller assigned node ID
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[3].p = (uint8_t *)&transfer_end_frame;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[4].v = sizeof(TRANSFER_END_FRAME);
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[6].v = 0; // No tx delay needed.
  p_mock_EnQueueSingleData_transfer_end_frame_remove->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock_EnQueueSingleData_transfer_end_frame_remove->return_code.v = true;

  invoke_tx_callback(p_mock_nop_controller_exclude_1, TRANSMIT_COMPLETE_NO_ACK, NULL);

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, &learn_mode_callback);
}

/*
 * Verifies the flow of a Smart Start inclusion of a Z-Wave Slave seen from the Controller's
 * perspective up until and including the transmission of an Assign IDs frame.
 */
void test_partial_inclusion_of_slave_with_smart_start(void)
{
//  TEST_IGNORE();
  mock_t * p_mock;

  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /* *********************************************************************************************
   * Initialization of Controller
   * *********************************************************************************************
   */

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */
  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  /* *********************************************************************************************
   * Actual test starts here
   * *********************************************************************************************
   */

  /*
   * Make Controller enter Smart Start add mode
   */
  ZW_AddNodeToNetwork(ADD_NODE_SMART_START, learn_mode_callback);

  /*
   * Generate a fake Smart Start Prime
   */
  inclusionHomeID[0] = 0xC8; //0xAA;
  inclusionHomeID[1] = 0xBB;
  inclusionHomeID[2] = 0xCC;
  inclusionHomeID[3] = 0xBA; //0xDD;
  crH[0] = inclusionHomeID[0];
  crH[1] = inclusionHomeID[1];
  crH[2] = inclusionHomeID[2];
  crH[3] = inclusionHomeID[3];
  const uint8_t smart_start_prime[] = {
                            ZWAVE_CMD_CLASS_PROTOCOL,             // cmdClass
                            ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO, // cmd
                            ZWAVE_NODEINFO_LISTENING_MASK | ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT, // capability
                            ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_SLAVE_ROUTING | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE, // security
                            ZWAVE_NODEINFO_BAUD_100K,             // reserved
                            GENERIC_TYPE_SWITCH_BINARY,           // nodeType generic
                            SPECIFIC_TYPE_NOT_USED,               // nodeType specific
                            COMMAND_CLASS_TRANSPORT_SERVICE_V2,   // CC index 0
                            COMMAND_CLASS_SECURITY_2,             // CC index 1
  };

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;
  rxOptions.isLongRangeChannel = false;
  rxOptions.rxStatus = RECEIVE_STATUS_SMART_NODE;
  rxOptions.homeId.array[0] = inclusionHomeID[0];
  rxOptions.homeId.array[1] = inclusionHomeID[1];
  rxOptions.homeId.array[2] = inclusionHomeID[2];
  rxOptions.homeId.array[3] = inclusionHomeID[3];

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &p_mock);
  p_mock->expect_arg[0].v = UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED;
  p_mock->expect_arg[1].v = 0;
  uint8_t expected_command[] = {
                                0xC8, //0xAA,
                                0xBB,
                                0xCC,
                                0xBA, //0xDD,
                                2, // nodeInfoLength
                                4, // Basic type translated from ZWAVE_NODEINFO_SLAVE_ROUTING to BASIC_TYPE_ROUTING_END_NODE
                                GENERIC_TYPE_SWITCH_BINARY,           // nodeType generic
                                SPECIFIC_TYPE_NOT_USED,               // nodeType specific
                                COMMAND_CLASS_TRANSPORT_SERVICE_V2,
                                COMMAND_CLASS_SECURITY_2
  };
  p_mock->expect_arg[2].p = &expected_command[0];
  p_mock->expect_arg[3].v = sizeof(expected_command);
  p_mock->return_code.v = true;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&smart_start_prime, &rxOptions));

  /*
   * Assuming the controller host recognizes the slave home ID, it will invoke
   * ZW_AddNodeDskToNetwork().
   */
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  uint8_t dsk[] = {
                   0x88, // | 0xC0 = 0xC8
                   0xBB,
                   0xCC,
                   0xBB, // & 0xFE = 0xBA
                   0xCC,
                   0xDD,
                   0xEE,
                   0xFF
  };
  ZW_AddNodeDskToNetwork(ADD_NODE_HOME_ID, &dsk[0], learn_mode_callback);

  /*
   * Generate a fake Smart Start Inclusion Request
   *
   * The Inclusion Request looks exactly like the Prime, but with another command.
   */
  uint8_t * const p_smart_start_inclusion_request = (uint8_t *)&smart_start_prime[0];
  *(p_smart_start_inclusion_request + 1) = ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;
  rxOptions.isLongRangeChannel = false;
  rxOptions.homeId.array[0] = inclusionHomeID[0];
  rxOptions.homeId.array[1] = inclusionHomeID[1];
  rxOptions.homeId.array[2] = inclusionHomeID[2];
  rxOptions.homeId.array[3] = inclusionHomeID[3];

  expect_assign_ids(0, 2, home_id.as_array);

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&smart_start_prime, &rxOptions));

  /*
   * Clean up before next test.
   */
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
  mock_call_use_as_stub(TO_STR(TimerStart));

  expect_zw_timer_register();
  expect_zw_timer_register();
  ZW_AddNodeToNetwork(ADD_NODE_STOP, learn_mode_callback);
}

/*
 * Verifies the flow of a Smart Start inclusion of a Z-Wave Slave seen from the Controller's
 * perspective up until and including the transmission of an Assign IDs frame.
 */
void test_partial_inclusion_of_lr_slave_with_smart_start(void)
{
//  TEST_IGNORE();
  mock_t * p_mock;

  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);


  /* *********************************************************************************************
   * Initialization of Controller
   * *********************************************************************************************
   */

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */
  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  /* *********************************************************************************************
   * Actual test starts here
   * *********************************************************************************************
   */

  /*
   * Make Controller enter Smart Start add mode
   */
  ZW_AddNodeToNetwork(ADD_NODE_SMART_START, learn_mode_callback);

  /*
   * Generate a fake Smart Start Prime
   */
  inclusionHomeID[0] = 0xC8; //0xAA;
  inclusionHomeID[1] = 0xBB;
  inclusionHomeID[2] = 0xCC;
  inclusionHomeID[3] = 0xBA; //0xDD;
  crH[0] = inclusionHomeID[0];
  crH[1] = inclusionHomeID[1];
  crH[2] = inclusionHomeID[2];
  crH[3] = inclusionHomeID[3];
  const uint8_t smart_start_prime[] = {
                            ZWAVE_CMD_CLASS_PROTOCOL_LR,              // cmdClass
                            ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO,  // cmd
                            ZWAVE_NODEINFO_LISTENING_MASK,            // capability
                            0,                                        // security
                            ZWAVE_NODEINFO_BAUD_100KLR,               // reserved
                            GENERIC_TYPE_SWITCH_BINARY,               // nodeType generic
                            SPECIFIC_TYPE_NOT_USED,                   // nodeType specific
                            2,                                        // CC count
                            COMMAND_CLASS_TRANSPORT_SERVICE_V2,       // CC index 0
                            COMMAND_CLASS_SECURITY_2,                 // CC index 1
  };

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;
  rxOptions.isLongRangeChannel = true;
  rxOptions.rxStatus = RECEIVE_STATUS_SMART_NODE;
  rxOptions.homeId.array[0] = inclusionHomeID[0];
  rxOptions.homeId.array[1] = inclusionHomeID[1];
  rxOptions.homeId.array[2] = inclusionHomeID[2];
  rxOptions.homeId.array[3] = inclusionHomeID[3];

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &p_mock);
  p_mock->expect_arg[0].v = UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED_LR;
  p_mock->expect_arg[1].v = 0;
  uint8_t expected_command[] = {
                                0xC8, //0xAA,
                                0xBB,
                                0xCC,
                                0xBA, //0xDD,
                                2, // nodeInfoLength
                                3, // Basic type translated from ZWAVE_NODEINFO_SLAVE_ROUTING to BASIC_TYPE_END_NODE
                                GENERIC_TYPE_SWITCH_BINARY,           // nodeType generic
                                SPECIFIC_TYPE_NOT_USED,               // nodeType specific
                                COMMAND_CLASS_TRANSPORT_SERVICE_V2,
                                COMMAND_CLASS_SECURITY_2
  };
  p_mock->expect_arg[2].p = &expected_command[0];
  p_mock->expect_arg[3].v = sizeof(expected_command);
  p_mock->return_code.v = true;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&smart_start_prime, &rxOptions));

  /*
   * Assuming the controller host recognizes the slave home ID, it will invoke
   * ZW_AddNodeDskToNetwork().
   */
  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  uint8_t dsk[] = {
                   0x88, // | 0xC0 = 0xC8
                   0xBB,
                   0xCC,
                   0xBB, // & 0xFE = 0xBA
                   0xCC,
                   0xDD,
                   0xEE,
                   0xFF
  };
  ZW_AddNodeDskToNetwork(ADD_NODE_OPTION_LR | ADD_NODE_HOME_ID, &dsk[0], learn_mode_callback);

  /*
   * Generate a fake Smart Start Inclusion Request
   *
   * The Inclusion Request looks exactly like the Prime, but with another command.
   */
  uint8_t smart_start_inclusion_request[sizeof(smart_start_prime)];
  memcpy(smart_start_inclusion_request, smart_start_prime, sizeof(smart_start_prime));
  smart_start_inclusion_request[1] = ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.sourceNode = 0;
  rxOptions.isLongRangeChannel = true;
  rxOptions.homeId.array[0] = inclusionHomeID[0];
  rxOptions.homeId.array[1] = inclusionHomeID[1];
  rxOptions.homeId.array[2] = inclusionHomeID[2];
  rxOptions.homeId.array[3] = inclusionHomeID[3];

  uint8_t new_node_id = 2;
  uint8_t * new_home_id = home_id.as_array;
  uint8_t * p_frame = frame_buffer;
  uint8_t i = 0;
  *(p_frame + i++) = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  *(p_frame + i++) = ZWAVE_LR_CMD_ASSIGN_IDS;
  *(p_frame + i++) = 0;
  *(p_frame + i++) = new_node_id;
  *(p_frame + i++) = *(new_home_id + 0);
  *(p_frame + i++) = *(new_home_id + 1);
  *(p_frame + i++) = *(new_home_id + 2);
  *(p_frame + i++) = *(new_home_id + 3);

  mock_call_expect(TO_STR(EnQueueSingleData), &p_mock);
  p_mock->expect_arg[0].v = RF_SPEED_LR_100K;
  p_mock->expect_arg[1].v = 1;
  p_mock->expect_arg[2].v = 0;
  p_mock->expect_arg[3].p = p_frame;
  p_mock->expect_arg[4].v = i;
  p_mock->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE | TRANSMIT_OPTION_DELAYED_TX | TRANSMIT_OPTION_LR_FORCE;
  p_mock->expect_arg[6].v = 100; // TRANSMIT_OPTION_DELAY_ASSIGN_ID_LR_MS
  p_mock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  p_mock->compare_rule_arg[8] = COMPARE_NOT_NULL;
  p_mock->return_code.v = true;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&smart_start_inclusion_request, &rxOptions));
}

/*
 * Verifies the frame flow when calling ZW_RequestNodeTypeNeighborUpdate.
 *
 * network setup 1 contorller( node 1)  2 LN (2, 5) and 2 FLIRS (nodes 3, 4)
 *
 * Host                  Controller                              Slave
 *  |-----API call node id 5->|                                    |
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<---------command_compelete---------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |----Get_nodes_in_range to node 3--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----------Node Range Info (5)------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<---------command_compelete---------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |----Get_nodes_in_range to node 4--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----------Node Range Info (5)------|
 *  |                         |-----------------ACK--------------->|
 *  |<------call_back---------|
 */
void test_ZW_RequestNodeTypeNeighborUpdate(void) {
//  TEST_IGNORE();

  mock_t * p_mock;

  // The node ID is read from NVM, but this happens in ZW_main and not in the controller module.
  // Hence, we must set the ID here because it's used in frames sent from the controller.
  g_nodeID = 1;

  // Stub setup
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(UpdateMostUsedNodes));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(llGetCurrentHeaderFormat));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(RangeInfoNeeded));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLineExile));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLinePurge));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(MergeNoneListeningNodes));
  mock_call_use_as_stub(TO_STR(RoutingInfoReceived));
  mock_call_use_as_stub(TO_STR(ClearPendingDiscovery));
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
  mock_call_use_as_stub(TO_STR(MaxCommonSpeedSupported));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer (via add node to network)
  expect_zw_timer_register(); // Replication timer (via add node to network)

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  node_id_t flirs_nodes[] = {0x03, 0x04};
  EX_NVM_NODEINFO node_in_network;
  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_LISTENING_SUPPORT);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY);
  node_in_network.reserved = ZWAVE_NODEINFO_BAUD_100K ;
  CtrlStorageSetNodeInfo_FAKE(2, &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(5, &node_in_network);

  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_BAUD_40000);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY|ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000);

  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[0], &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[1], &node_in_network);


  node_id_t ln_nodeid = 5;
  mock_t *p_mock_find_nodes_in_range;
  mock_t *p_mock_get_nodes_in_range;
  uint8_t nodemask[] = {0x10};  // node id 5

  COMMAND_COMPLETE_FRAME command_complete_frame = {
      .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
      .cmd = ZWAVE_CMD_CMD_COMPLETE,
      .seqNo = 0};

  const uint8_t range_info_frame[] = {
      ZWAVE_CMD_CLASS_PROTOCOL,
      ZWAVE_CMD_RANGE_INFO,
      1,
      0x10,
      0};


  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[0], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));

  expect_timer_set_callback(); // assign_ID.TimeoutTimer


  STransmitCallback TxCallback = { .pCallback = NeighborUpdate_callback, .Context = 0 };
  uint8_t call_result = ZW_RequestNodeTypeNeighborUpdate(ln_nodeid, E_SYSTEM_TYPE_FLIRS, &TxCallback);


  rxOptions.sourceNode = 0;

  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[0], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer
  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[0];
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));
  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[1], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer
  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));

  rxOptions.sourceNode = 0;
  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[1], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[1];

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));
  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

   mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
   p_mock->expect_arg[0].v = ln_nodeid;
   p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
   p_mock->expect_arg[2].v = (ZW_GET_ROUTING_INFO_ANY|GET_ROUTING_INFO_REMOVE_NON_REPS);
  uint8_t neighbors[MAX_NODEMASK_LENGTH];
  memset(neighbors, 0, MAX_NODEMASK_LENGTH);
  neighbors[0] = 0xC0; // node 5 has nodes 3 and 4 as neighbors
  p_mock->output_arg[1].p = neighbors;


  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));
  TEST_ASSERT_TRUE(call_result);
}



/*
 * Verifies the frame flow when calling ZW_RequestNodeTypeNeighborUpdate.
 * no command complete is received from node 3
 * network setup 1 contorller( node 1)  2 LN (2, 5) and 2 FLIRS (nodes 3, 4)
 *
 * Host                  Controller                              Slave
 *  |-----API call node id 5->|                                    |
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |----find_node_in_range_timeout -----| (callback)
 *  |                         |----Get_nodes_in_range to node 3--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----------Node Range Info (5)------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<---------command_compelete---------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |----Get_nodes_in_range to node 4--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----------Node Range Info (5)------|
 *  |                         |-----------------ACK--------------->|
 *  |<------call_back---------|
 */
void test_ZW_RequestNodeTypeNeighborUpdate_cmd_compelete(void) {
//  TEST_IGNORE();

  mock_t * p_mock;

  // The node ID is read from NVM, but this happens in ZW_main and not in the controller module.
  // Hence, we must set the ID here because it's used in frames sent from the controller.
  g_nodeID = 1;

  // Stub setup
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(UpdateMostUsedNodes));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(llGetCurrentHeaderFormat));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(RangeInfoNeeded));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLineExile));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLinePurge));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(MergeNoneListeningNodes));
  mock_call_use_as_stub(TO_STR(RoutingInfoReceived));
  mock_call_use_as_stub(TO_STR(ClearPendingDiscovery));
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
  mock_call_use_as_stub(TO_STR(MaxCommonSpeedSupported));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer (via add node to network)
  expect_zw_timer_register(); // Replication timer (via add node to network)

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  node_id_t flirs_nodes[] = {0x03, 0x04};
  EX_NVM_NODEINFO node_in_network;
  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_LISTENING_SUPPORT);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY);
  node_in_network.reserved = ZWAVE_NODEINFO_BAUD_100K ;
  CtrlStorageSetNodeInfo_FAKE(2, &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(5, &node_in_network);

  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_BAUD_40000);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY|ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000);

  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[0], &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[1], &node_in_network);


  node_id_t ln_nodeid = 5;
  mock_t *p_mock_find_nodes_in_range;
  mock_t *p_mock_find_node_timeout;
  mock_t *p_mock_get_nodes_in_range;
  uint8_t nodemask[] = {0x10};  // node id 5

  COMMAND_COMPLETE_FRAME command_complete_frame = {
      .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
      .cmd = ZWAVE_CMD_CMD_COMPLETE,
      .seqNo = 0};

  const uint8_t range_info_frame[] = {
      ZWAVE_CMD_CLASS_PROTOCOL,
      ZWAVE_CMD_RANGE_INFO,
      1,
      0x10,
      0};


  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[0], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));

  p_mock_find_node_timeout = expect_timer_set_callback(); // assign_ID.TimeoutTimer


  STransmitCallback TxCallback = { .pCallback = NeighborUpdate_callback, .Context = 0 };
  uint8_t call_result = ZW_RequestNodeTypeNeighborUpdate(ln_nodeid, E_SYSTEM_TYPE_FLIRS, &TxCallback);


  rxOptions.sourceNode = 0;

  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[0], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer
  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[0];
  invoke_timer_callback(p_mock_find_node_timeout);

  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_NO_ACK, NULL);

  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[1], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer


  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));

  rxOptions.sourceNode = 0;
  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[1], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[1];

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));
  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

   mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
   p_mock->expect_arg[0].v = ln_nodeid;
   p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
   p_mock->expect_arg[2].v = (ZW_GET_ROUTING_INFO_ANY|GET_ROUTING_INFO_REMOVE_NON_REPS);
   uint8_t neighbors[MAX_NODEMASK_LENGTH];
   memset(neighbors, 0, MAX_NODEMASK_LENGTH);
   neighbors[0] = 0xC0; // node 5 has nodes 3 and 4 as neighbors
   p_mock->output_arg[1].p = neighbors;

  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));
  TEST_ASSERT_TRUE(call_result);
}


/*
 * Verifies the frame flow when calling ZW_RequestNodeTypeNeighborUpdate.
 *
 * We don't recie range info from node 3
 *
 * network setup 1 contorller( node 1)  2 LN (2, 5) and 2 FLIRS (nodes 3, 4)
 *
 * Host                  Controller                              Slave
 *  |-----API call node id 5->|                                    |
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<---------command_compelete---------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |----Get_nodes_in_range to node 3--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----Node Range Info time out-- ----|
 *  |                         |--find_node_in_range (5) to node 3->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<---------command_compelete---------|
 *  |                         |-----------------ACK--------------->|
 *  |                         |----Get_nodes_in_range to node 4--->|
 *  |                         |<----------------ACK----------------| (callback)
 *  |                         |<----------Node Range Info (5)------|
 *  |                         |-----------------ACK--------------->|
 *  |<------call_back---------|
 */
void test_ZW_RequestNodeTypeNeighborUpdate_get_range_info_timeout(void) {
//  TEST_IGNORE();

  mock_t * p_mock;

  // The node ID is read from NVM, but this happens in ZW_main and not in the controller module.
  // Hence, we must set the ID here because it's used in frames sent from the controller.
  g_nodeID = 1;

  // Stub setup
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(ExploreTransmitSetNWIMode));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(UpdateMostUsedNodes));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(llGetCurrentHeaderFormat));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(RangeInfoNeeded));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLineExile));
  mock_call_use_as_stub(TO_STR(LastWorkingRouteCacheLinePurge));
  mock_call_use_as_stub(TO_STR(ClearMostUsed));
  mock_call_use_as_stub(TO_STR(NvmFileSystemFormat));
  mock_call_use_as_stub(TO_STR(TxQueueInit));
  mock_call_use_as_stub(TO_STR(MergeNoneListeningNodes));
  mock_call_use_as_stub(TO_STR(RoutingInfoReceived));
  mock_call_use_as_stub(TO_STR(ClearPendingDiscovery));
  mock_call_use_as_stub(TO_STR(RestartAnalyseRoutingTable));
  mock_call_use_as_stub(TO_STR(MaxCommonSpeedSupported));
#ifndef ZW_CONTROLLER_BRIDGE
  mock_call_use_as_stub(TO_STR(PreferredSet));
#endif
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Initializing the CC List is required. Otherwise we address non-existing memory causing a
   * segmentation fault.
   */
  uint8_t command_classes[NODEPARM_MAX];
  for (uint8_t i = 0; i < NODEPARM_MAX; i++)
  {
    command_classes[i] = i + 1;
  }
  SCommandClassSet_t CCSet = {
                                    .UnSecureIncludedCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedUnSecureCC = {sizeof(command_classes), command_classes},
                                    .SecureIncludedSecureCC = {sizeof(command_classes), command_classes}
  };

  const SNetworkInfo NetworkInfo = {
    .eInclusionState = EINCLUSIONSTATE_EXCLUDED
  };
  CCListSetNetworkInfo(&NetworkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &p_mock);
  p_mock->return_code.p = &CCSet;

  /*
   * Initialize Controller
   */

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }
  expect_zw_timer_register(); // Presentation timer (via add node to network)
  expect_zw_timer_register(); // Replication timer (via add node to network)

  SAppNodeInfo_t ControllerNodeInfo;
  // DeviceOptionsMask is required in order to write the correct controller configuration to NVM
  ControllerNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  ControllerNodeInfo.NodeType.generic = GENERIC_TYPE_STATIC_CONTROLLER;
  ControllerNodeInfo.NodeType.specific = SPECIFIC_TYPE_PC_CONTROLLER;
  ControllerInit(NULL, &ControllerNodeInfo, true);

  /*
   * ControllerInit() MUST be invoked before ZW_SetDefault() because ControllerNodeInfo is saved
   * and used for the invocation of ControllerInit() in ZW_SetDefault().
   */
  mock_call_expect(TO_STR(HomeIdGeneratorGetNewId), &p_mock);
  p_mock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  union {
    uint8_t as_array[4];
    uint32_t as_uint32;
  } home_id;
  home_id.as_uint32 = 0xC1234567;
  p_mock->output_arg[0].p = home_id.as_array;
  p_mock->return_code.v = home_id.as_uint32;

  for (uint32_t i = 0; i < TIMER_COUNT; i++) {
    expect_zw_timer_register();
  }

  expect_zw_timer_register(); // Presentation timer
  expect_zw_timer_register(); // Replication timer

  ZW_SetDefault(); // Set default is required to reset node ID pool

  node_id_t flirs_nodes[] = {0x03, 0x04};
  EX_NVM_NODEINFO node_in_network;
  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_LISTENING_SUPPORT);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY);
  node_in_network.reserved = ZWAVE_NODEINFO_BAUD_100K ;
  CtrlStorageSetNodeInfo_FAKE(2, &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(5, &node_in_network);

  node_in_network.capability = (ZWAVE_NODEINFO_VERSION_4|ZWAVE_NODEINFO_ROUTING_SUPPORT|ZWAVE_NODEINFO_BAUD_40000);
  node_in_network.security = (ZWAVE_NODEINFO_SLAVE_ROUTING|ZWAVE_NODEINFO_BEAM_CAPABILITY|ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000);

  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[0], &node_in_network);
  CtrlStorageSetNodeInfo_FAKE(flirs_nodes[1], &node_in_network);


  node_id_t ln_nodeid = 5;
  mock_t *p_mock_find_nodes_in_range;
  mock_t *p_mock_get_nodes_in_range;
  mock_t *p_mock_range_info_timeout;
  uint8_t nodemask[] = {0x10};  // node id 5

  COMMAND_COMPLETE_FRAME command_complete_frame = {
      .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
      .cmd = ZWAVE_CMD_CMD_COMPLETE,
      .seqNo = 0};

  const uint8_t range_info_frame[] = {
      ZWAVE_CMD_CLASS_PROTOCOL,
      ZWAVE_CMD_RANGE_INFO,
      1,
      0x10,
      0};


  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[0], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));

  expect_timer_set_callback(); // assign_ID.TimeoutTimer


  STransmitCallback TxCallback = { .pCallback = NeighborUpdate_callback, .Context = 0 };
  uint8_t call_result = ZW_RequestNodeTypeNeighborUpdate(ln_nodeid, E_SYSTEM_TYPE_FLIRS, &TxCallback);


  rxOptions.sourceNode = 0;

  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[0], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  p_mock_range_info_timeout = expect_timer_set_callback(); // assign_ID.TimeoutTimer
  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[0];
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));
  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_find_nodes_in_range = expect_find_nodes_in_range_v2(flirs_nodes[1], nodemask, sizeof(nodemask), 0, ZWAVE_FIND_NODES_SPEED_100K,  (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  invoke_timer_callback(p_mock_range_info_timeout);

  rxOptions.sourceNode = 0;
  invoke_tx_callback(p_mock_find_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  p_mock_get_nodes_in_range = expect_get_nodes_in_range(flirs_nodes[1], (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE));
  expect_timer_set_callback(); // assign_ID.TimeoutTimer

  /*
   * Generate a "Command Complete" frame and pass it to CommandHandler()
  */
  rxOptions.sourceNode = flirs_nodes[1];

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&command_complete_frame, &rxOptions));
  invoke_tx_callback(p_mock_get_nodes_in_range, TRANSMIT_COMPLETE_OK, NULL);

  /*
   *
   * Generate a "Range Info" frame and pass it to CommandHandler()
   *
   */
   mock_call_expect(TO_STR(ZW_GetRoutingInfo), &p_mock);
   p_mock->expect_arg[0].v = ln_nodeid;
   p_mock->compare_rule_arg[1] = COMPARE_NOT_NULL;
   p_mock->expect_arg[2].v = (ZW_GET_ROUTING_INFO_ANY|GET_ROUTING_INFO_REMOVE_NON_REPS);
   uint8_t neighbors[MAX_NODEMASK_LENGTH];
   memset(neighbors, 0, MAX_NODEMASK_LENGTH);
   neighbors[0] = 0xC0; // node 5 has nodes 3 and 4 as neighbors
   p_mock->output_arg[1].p = neighbors;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&range_info_frame, &rxOptions));
  TEST_ASSERT_TRUE(call_result);
}
