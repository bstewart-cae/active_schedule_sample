// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_ismyframe_CONTROLLER.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>

#include "ZW_ismyframe.h"
#include "ZW_explore.h"
#include "ZW_controller.h"
#include "ZW_controller_network_info_storage.h"
#include "ZW_Frame.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

//#define PRINT_TESTFRAME
#define sizeof_array(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

          /** START of Global variable needed for testing **/

bool g_learnModeClassic;
bool g_learnMode;
bool exploreInclusionModeRepeat;
uint8_t crH[HOMEID_LENGTH];
uint8_t currentSeqNo;
uint8_t sACK;
bool bNetworkWideInclusionReady = false;
uint8_t inclusionHomeID[HOMEID_LENGTH];
uint8_t inclusionHomeIDActive[HOMEID_LENGTH];
uint8_t bNetworkWideInclusion;
uint8_t mTransportRxCurrentSpeed;
node_id_t virtualNodeID;         /* Current virtual nodeID */
uint8_t learnSlaveMode;
bool virtualNodeDest;
uint8_t g_learnNodeState;
ASSIGN_ID assign_ID;
uint8_t bMaxNodeID;
extern TxQueueElement *pFrameWaitingForACK;

          /** END of Global variable needed for testing **/

uint8_t raw_ack_frame_LR[]        = { 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x0A, 0x03, 0x01 };

uint8_t raw_singlecast_frame_LR[] = { 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x24, 0x01, 0x00, 0xAB, 0xAB, 0x01,
                                      0x01, 0xD3, 0x9C, 0x03, 0x10, 0x00, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86,
                                      0x72, 0x5A, 0x87, 0x73, 0x98, 0x9F, 0x6C, 0x7A, 0xA1, 0x71 };

uint8_t raw_smartstart_frame_LR[] = { 0xDB, 0x04, 0xED, 0x2A, 0x00, 0x0F, 0xFF, 0x24, 0x01,
                                      0x0A, 0xAB, 0xBA, 0x01, 0x27, 0xD3, 0x9C, 0x03, 0x10,
                                      0x00, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72,
                                      0x5A, 0x87, 0x73, 0x98, 0x9F, 0x6C, 0x7A, 0x35, 0x49};  // smart start prime

RX_FRAME testRxFrame; // input parameter of test target function isMyFrame()
ZW_ReceiveFrame_t pReceiveFrame_tmp;
TxQueueElement toCmpAck;
uint16_t source_id = 0x456;
uint16_t dest_id   = 0x789;
uint16_t dest_id_broadcast = 0xFFF;

uint8_t * pReceiveStatus = 0;

bool yesItIs;


static void initCommonVariableLR(void) {

  /** Default values are set here. The test can change the values if required **/
  pReceiveFrame_tmp.frameOptions.sequenceNumber = 0x01;

  toCmpAck.frame.frameOptions.routed = 0;
  toCmpAck.frame.frameOptions.sequenceNumber = 0x01;
  toCmpAck.frame.profile = RF_PROFILE_100K_LR_A;

  testRxFrame.pReceiveFrame = &pReceiveFrame_tmp;
  pFrameWaitingForACK = &toCmpAck;

  /** Initialize the global variable used by the isMyFrame **/
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xC9;
  homeID[1] = 0x0C;
  homeID[2] = 0x6C;
  homeID[3] = 0x72;
  ZW_HomeIDSet(homeID);

  g_nodeID = dest_id;
  /** END of Initialize the global variable used by the isMyFrame **/
}

static void initTestFrame_LR(uint8_t *home_id, uint16_t source_id, uint16_t dest_id, uint8_t header_type, uint8_t *frame) {

  uint8_t source_dest_id[3] = { (source_id >> 4), (source_id << 4) | (dest_id >> 8), dest_id };
  
  // initialize homeID
  for(uint8_t i=0; i<HOMEID_LENGTH; i++)
    frame[i] = home_id[i];

  // initialize source and destination IDs
  for(uint8_t i=0; i<3; i++)
    frame[HOMEID_LENGTH + i] = source_dest_id[i];

  // initialize frame header type
  frame[8] = (frame[8] & ~(0x07)) | (header_type & 0x07);

#ifdef PRINT_TESTFRAME
  printf("Test Frame: \n");
  for(int i=0; i< frame[7]; i++) {
    printf("0x%x ", frame[i]);
  }
  printf("\n");
#endif

  return;
}

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
}

void tearDown(void)
{
  mock_calls_verify();
}

typedef struct
{
  bool expected_result;
  uint8_t  homeID[HOMEID_LENGTH];
  uint16_t source_id;
  uint16_t dest_id;
  ZW_FrameType_t frameType;
  uint8_t * pFrame;
}
test_vector_t;

void test_ZW_ismyframe_LR(void)
{
  test_vector_t test_vectors[] = {
    {true,  {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id_broadcast, HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //0 - FRAME SINGLECAST  - broadcast
    {true,  {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id_broadcast, HDRTYP_SINGLECAST,     raw_smartstart_frame_LR}, //1 - FRAME SMART START - broadcast
    {true,  {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //2 - FRAME SINGLECAST
    {true,  {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_SINGLECAST,     raw_smartstart_frame_LR}, //3 - FRAME SMART START
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       (uint16_t)0x123  , HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //4 - FRAME SINGLECAST - nodeID != dest_id
    {false, { 0x1,  0x2,  0x3,  0x4}, source_id,       dest_id          , HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //5 - FRAME SINGLECAST - wrong homeID
    {false, {0xC9, 0x0C, 0x6C, 0x72}, (uint16_t)0xFA1, dest_id          , HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //6 - FRAME SINGLECAST - with reserved sourceID (0xFA1 - 0xFFE)
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       (uint16_t)0xFA1  , HDRTYP_SINGLECAST,     raw_singlecast_frame_LR}, //7 - FRAME SINGLECAST - with reserved destID (0xFA1 - 0xFFE)
    {true,  {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_TRANSFERACK,    raw_ack_frame_LR},        //8 - FRAME ACK - dest = 0x789 (12bits)
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_MULTICAST,      raw_singlecast_frame_LR}, //9 - FRAME MULTICAST - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_FLOODED,        raw_singlecast_frame_LR}, //10 - FRAME FLOODED - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_EXPLORE,        raw_singlecast_frame_LR}, //11 - FRAME EXPLORE - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYPE_AVFRAME,       raw_singlecast_frame_LR}, //12 - FRAME AVFRAME - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYPE_AVACKNOWLEDGE, raw_singlecast_frame_LR}, //13 - FRAME AVACKNOWLEDGE - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_ROUTED,         raw_singlecast_frame_LR}, //14 - FRAME ROUTED - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_MULTICAST,      raw_singlecast_frame_LR}, //15 - FRAME MULTICAST - not allowed for LR
    {false, {0xC9, 0x0C, 0x6C, 0x72}, source_id,       dest_id          , HDRTYP_MULTICAST,      raw_singlecast_frame_LR}  //16 - FRAME MULTICAST - not allowed for LR
  };
  bool result;
  mock_t * p_mock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(TxQueueBeamACKReceived));

  for (uint32_t i = 0; i < sizeof_array(test_vectors); i++)
  {
    printf("Test vector #%u\n", i);
    initCommonVariableLR();

    testRxFrame.pFrame = (frame *)test_vectors[i].pFrame;

    initTestFrame_LR( test_vectors[i].homeID,  test_vectors[i].source_id,  test_vectors[i].dest_id, test_vectors[i].frameType, test_vectors[i].pFrame);

    // Does IsMyFrame() need to call ZW_IsVirtualNode() ?
    if ((test_vectors[i].dest_id != dest_id_broadcast) && (test_vectors[i].dest_id != g_nodeID))
    {
      // Then setup a mock for ZW_IsVirtualNode()
      mock_call_expect(TO_STR(ZW_IsVirtualNode),&p_mock);
      p_mock->expect_arg[ARG0].value = test_vectors[i].dest_id;
      p_mock->return_code.value = false;
    }

    result = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
    printf("Expected: %i, actual: %i \n", test_vectors[i].expected_result, result);

    TEST_ASSERT_MESSAGE(test_vectors[i].expected_result == result, "ZW_ismyframe() did not return the right value!");
  }

  mock_calls_verify();
}

/********************************************* FRAME ACK - broadcast (not valid!) */
void test_isMyFrame_LR_broadcast_transferack(void)
{
  mock_t * p_mock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(TxQueueBeamACKReceived));

  initCommonVariableLR();
  testRxFrame.pFrame = (frame *)raw_ack_frame_LR;
  initTestFrame_LR(ZW_HomeIDGet(), source_id, dest_id_broadcast, HDRTYP_TRANSFERACK, raw_ack_frame_LR);

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&p_mock);
  p_mock->expect_arg[ARG0].value = dest_id_broadcast;
  p_mock->return_code.value = false;

  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);

  mock_calls_verify();
}
/*********************************************  */

/********************************************** FRAME SINGLECAST - using a 2CH frame */
void test_isMyFrame_LR_with_2ch_frame(void)
{
  initCommonVariableLR();
  uint8_t raw_singlecast_frame_2CH[] = { 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0x01, 0x09, 0x0D, 0xFF, 0x01, 0x08, 0x05, 0xAC};
  testRxFrame.pFrame = (frame *)raw_singlecast_frame_2CH;

  /** Initialize the global variable used by the isMyFrame **/
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  g_nodeID = 0x1;
  /** END of Initialize the global variable used by the isMyFrame **/
  
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/********************************************** FRAME SINGLECAST in learn mode classic inclusion - using a 2CH frame */


//void completedFunc(LEARN_INFO_T* info){}  // Part of an unused solution model below. Exists as an example.

/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_2CH_src_2_dest_3(void)
{
  printf("\n\n\n");
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(NODEINFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x02, 0x01, 0x09, 0x0D, 0x03, 0x01, 0x08, 0x05, 0xAC};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);  // Not easy to use since it will involve too much.
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 3);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  NODEINFO_FRAME* payload = (NODEINFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_2CH_src_2_dest_1(void)
{
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(NODEINFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x02, 0x01, 0x09, 0x0D, 0x01, 0x01, 0x08, 0x05, 0xAC};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 1);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  NODEINFO_FRAME* payload = (NODEINFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_REQUEST_NODE_INFO_frame_2CH_src_1_dest_2(void)
{
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(REQ_NODE_INFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|   PAYLOAD   |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x09, 0x0C, 0x02, 0x01, 0x08, 0x05};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 1);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  REQ_NODE_INFO_FRAME* payload = (REQ_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_REQUEST_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_REQUEST_NODE_INFO_frame_2CH_src_2_dest_1(void)
{
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(REQ_NODE_INFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|   PAYLOAD   |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x02, 0x01, 0x09, 0x0C, 0x01, 0x01, 0x08, 0x05};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 1);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  REQ_NODE_INFO_FRAME* payload = (REQ_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_REQUEST_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_TRANSFER_NODE_INFO_frame_2CH_src_1_dest_2(void)
{
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(TRANSFER_NODE_INFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x09, 0x0D, 0x02, 0x01, 0x08, 0x05, 0xAC};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 1);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  TRANSFER_NODE_INFO_FRAME* payload = (TRANSFER_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_TRANSFER_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (CLS) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_TRANSFER_NODE_INFO_frame_2CH_src_2_dest_1(void)
{
  initCommonVariableLR();
  toCmpAck.frame.profile = RF_PROFILE_100K;  // 2CH

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_2CH[sizeof(frameSinglecast) + sizeof(TRANSFER_NODE_INFO_FRAME)]
  /*     |        HomeID        |source|hInfo|res |length|dest| SOP|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x09, 0x0D, 0x02, 0x01, 0x08, 0x05, 0xAC};

  testRxFrame.pFrame = (frame*)raw_singlecast_frame_2CH;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 2);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, 1);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set command class of frame.
  TRANSFER_NODE_INFO_FRAME* payload = (TRANSFER_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  payload->cmd = ZWAVE_CMD_TRANSFER_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_LR_src_1_dest_3(void)  // This is an unreal tx in LR, due to nodeID range.
{
  printf("\n\n\n");
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(NODEINFO_LR_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x00, 0x10, 0x03,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                        OK                 OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 1);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 3);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  NODEINFO_LR_FRAME* payload = (NODEINFO_LR_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_LR_src_2_dest_1(void)  // This is an unreal tx in LR, due to nodeID range.
{
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(NODEINFO_LR_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x00, 0x20, 0x01,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 2);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 1);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  NODEINFO_LR_FRAME* payload = (NODEINFO_LR_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_LR_src_256_dest_3(void)
{
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(NODEINFO_LR_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x10, 0x00, 0x03,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 256);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 3);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  NODEINFO_LR_FRAME* payload = (NODEINFO_LR_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (LR) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_NODE_INFO_frame_LR_src_256_dest_1(void)
{
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(NODEINFO_LR_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x10, 0x00, 0x01,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x05;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 256);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 1);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  NODEINFO_LR_FRAME* payload = (NODEINFO_LR_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (LR) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_REQUEST_NODE_INFO_frame_LR_src_1_dest_2(void)
{
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(REQ_NODE_INFO_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x00, 0x10, 0x02,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 1);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 2);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  REQ_NODE_INFO_FRAME* payload = (REQ_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_REQUEST_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}


/**
 * This will test whether the controller in learnmode (LR) successfully drops packets belonging to a foreign network.
 *
 * The controller node has homeID (A5 A5 A5 A5), the foreign network has (AA AA AA AA).
 */
void test_isMyFrame_learnmode_ADD_foreign_nwk_REQUEST_NODE_INFO_frame_LR_src_1_dest_256(void)
{
  initCommonVariableLR();

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(REQ_NODE_INFO_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xAA, 0xAA, 0xAA, 0xAA,      0x00, 0x11, 0x00,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)raw_singlecast_frame_LR;

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xA5;
  homeID[1] = 0xA5;
  homeID[2] = 0xA5;
  homeID[3] = 0xA5;
  ZW_HomeIDSet(homeID);

  // Set controller into learnmode classic inclusion. (ADD button on PC Controller)
  g_learnMode = false;  // The node itself is not about to be included.
  g_learnNodeState = LEARN_NODE_STATE_NEW;  // We are adding node to the network.
//  ZW_AddNodeToNetwork(ADD_NODE_ANY, completedFunc);
  // Set the node's own nodeID.
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 1);
  // Set destination nodeID.
  SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, 256);
  // Set foreign homeID.
  // AA AA AA AA. See raw_singlecast_frame_2CH above.

  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);

  // Set command class of frame.
  REQ_NODE_INFO_FRAME* payload = (REQ_NODE_INFO_FRAME*)GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  payload->cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  payload->cmd = ZWAVE_LR_CMD_REQUEST_NODE_INFO;

  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}

typedef struct
{
  bool expected_result;
  uint8_t cmdClass;
  uint8_t cmd;
  uint16_t dest_id;
}
test_vector_1_t;

/**
 * This will test whether the controller accpet protocol commands with broadcast destination id.
 *
 */
void test_ismyframe_lr_protocol_command_with_broadcast_dest_id(void)
{
  initCommonVariableLR();
  g_learnModeClassic = false;;
  bNetworkWideInclusionReady = false;
  bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

  test_vector_1_t test_vectors [] = {{true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_NODE_INFO                       , NODE_BROADCAST_LR},
                                     {false, ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_TRANSFER_PRESENTATION           , NODE_BROADCAST_LR},
                                     {false, ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_SET_NWI_MODE                    , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_INCLUDED_NODE_INFO              , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO      , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO    , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_EXCLUDE_REQUEST                 , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_LR_CMD_NODE_INFO                    , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_LR_CMD_INCLUDED_NODE_INFO           , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO   , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_LR_CMD_EXCLUDE_REQUEST              , NODE_BROADCAST_LR}, 
                                     {false, ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_NOP_POWER                       , NODE_BROADCAST_LR},
                                     {true , ZWAVE_CMD_CLASS_PROTOCOL_LR, ZWAVE_CMD_NOP_POWER                       , 1},
                                     {true , 0x34                       , ZWAVE_CMD_NOP_POWER                       , 1},
                                     {true , 0x34                       , ZWAVE_CMD_NOP_POWER                       , NODE_BROADCAST_LR},
                                    };

  /**
   * Create and initialize the initial frame for later modifications.
   */
  uint8_t my_raw_singlecast_frame_LR[sizeof(frameHeaderSinglecastLR) + sizeof(REQ_NODE_INFO_FRAME)]
  /*     |        HomeID        |12bits sourceID|12bits destID|length|hInfo|seqNbr|noise|txPwr|     PAYLOAD     |*/
      = { 0xC0, 0x55, 0x55, 0x55,      0x00, 0x11, 0x00,        0x0D, 0x03,  0x01, 0x00, 0x00, 0xAC};
  /*                OK                         OK                OK    OK?    OK    OK?   OK     ?      */
  testRxFrame.pFrame = (frame*)my_raw_singlecast_frame_LR;
  testRxFrame.pFrame->headerLR.length = 20; // use dummy length that longer than the header length

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xC0;
  homeID[1] = 0x55;
  homeID[2] = 0x55;
  homeID[3] = 0x55;
  ZW_HomeIDSet(homeID);

  g_learnMode = false;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE_LR(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_LR(*testRxFrame.pFrame, 258);
  // Set sequence number for completeness
  SET_SEQNUMBER_LR(*testRxFrame.pFrame, 1);
  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  // Set command class of frame.
  uint8_t* payload = GetSinglecastPayload(HDRFORMATTYP_LR, testRxFrame.pFrame);
  for (uint8_t i=0; i< sizeof_array(test_vectors); i++) {
    printf("Test vector #%u\n", i);
    // Set destination nodeID.
    SET_SINGLECAST_DESTINATION_NODEID_LR(*testRxFrame.pFrame, test_vectors[i].dest_id);
    payload[0] = test_vectors[i].cmdClass;
    payload[1] = test_vectors[i].cmd;
    /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
    yesItIs = IsMyFrame(HDRFORMATTYP_LR, &testRxFrame, pReceiveStatus);
    printf("Expected: %i, actual: %i \n", test_vectors[i].expected_result, yesItIs);
    TEST_ASSERT_MESSAGE(test_vectors[i].expected_result == yesItIs, "ZW_ismyframe() did not return the right value!");
  }

}


void test_ismyframe_protocol_command_with_broadcast_dest_id(void)
{
  initCommonVariableLR();
  uint8_t raw_singlecast_frame_2CH[] = { 0xC0, 0x55, 0x55, 0x55, 0x55, 0x01, 0x09, 0x0D, 0xFF, 0x01, 0x08, 0x05, 0xAC};
  testRxFrame.pFrame = (frame *)raw_singlecast_frame_2CH;
  g_learnModeClassic = false;;
  bNetworkWideInclusionReady = false;
  bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

  test_vector_1_t test_vectors [] = {{true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_NODE_INFO                    , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_TRANSFER_PRESENTATION        , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_SET_NWI_MODE                 , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_INCLUDED_NODE_INFO           , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO   , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_EXCLUDE_REQUEST              , NODE_BROADCAST},
                                     {false , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_NOP_POWER                    , NODE_BROADCAST},
                                     {true  , ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_NOP_POWER                    , 1},
                                     {true  , 0x34                    , ZWAVE_CMD_NOP_POWER                    , 1},
                                     {true  , 0x34                    , ZWAVE_CMD_NOP_POWER                    , NODE_BROADCAST},
                                    };

  testRxFrame.pFrame->headerLR.length = 20; // use dummy length that longer than the header length

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  homeID[0] = 0xC0;
  homeID[1] = 0x55;
  homeID[2] = 0x55;
  homeID[3] = 0x55;
  ZW_HomeIDSet(homeID);

  g_learnMode = false;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_nodeID = 0x01;

  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_SINGLECAST);
  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 5);
  /**
   * Execute IsMyFrame() and evaluate the outcome.
   */

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));

  // Set command class of frame.
  uint8_t* payload = GetSinglecastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  for (uint8_t i=0; i< sizeof_array(test_vectors); i++) {
    printf("Test vector #%u\n", i);
    // Set destination nodeID.
    SET_SINGLECAST_DESTINATION_NODEID_2CH(*testRxFrame.pFrame, test_vectors[i].dest_id);
    payload[0] = test_vectors[i].cmdClass;
    payload[1] = test_vectors[i].cmd;
    /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
    yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
    printf("Expected: %i, actual: %i \n", test_vectors[i].expected_result, yesItIs);
    TEST_ASSERT_MESSAGE(test_vectors[i].expected_result == yesItIs, "ZW_ismyframe() did not return the right value!");
  }
}

void test_ismyframe_protocol_command_using_multicast_header(void)
{
  initCommonVariableLR();
  uint8_t raw_singlecast_frame_2CH[170] = { 0xC0, 0x55, 0x55, 0x55, 0x55, 0x01, 0x09, 0x0D, 0xFF, 0x01, 0x08, 0x05, 0xAC};
  testRxFrame.pFrame = (frame *)raw_singlecast_frame_2CH;
  bNetworkWideInclusionReady = false;
  bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
  testRxFrame.pFrame->headerLR.length = 50; // use dummy length that longer than the header length

  /**
   * Initialize the global variables used by the IsMyFrame().
   */

  // Set the node's own homeID to A5 A5 A5 A5.
  uint8_t homeID[HOMEID_LENGTH];
  uint8_t* payload;
  homeID[0] = 0xC0;
  homeID[1] = 0x55;
  homeID[2] = 0x55;
  homeID[3] = 0x55;
  ZW_HomeIDSet(homeID);

  g_learnMode = false;
  g_nodeID = 0x01;

  mock_call_use_as_stub(TO_STR(ZW_IsVirtualNode));
  // Set frame header type to Singlecast
  SET_HEADERTYPE(*testRxFrame.pFrame, HDRTYP_MULTICAST);

  // Set source nodeID.
  SET_SINGLECAST_SOURCE_NODEID_2CH(*testRxFrame.pFrame, 5);
  printf("Testing 2ch mutlicast protocol frame\n" );
  // Set command class of frame.
  payload = GetMulticastPayload(HDRFORMATTYP_2CH, testRxFrame.pFrame);
  payload[0] = ZWAVE_CMD_CLASS_PROTOCOL;
  payload[1] = ZWAVE_CMD_NOP_POWER;
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_2CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);

  SET_SINGLECAST_SOURCE_NODEID_3CH(*testRxFrame.pFrame, 5);
  printf("Testing 3ch mutlicast protocol frame\n" );
  // Set command class of frame.
  payload = GetMulticastPayload(HDRFORMATTYP_3CH, testRxFrame.pFrame);
  payload[0] = ZWAVE_CMD_CLASS_PROTOCOL;
  payload[1] = ZWAVE_CMD_NOP_POWER;
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  yesItIs = IsMyFrame(HDRFORMATTYP_3CH, &testRxFrame, pReceiveStatus);
  TEST_ASSERT_FALSE(yesItIs);
}
