/**
 * @file test_CC_Supervision.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 *  Not tested:
 *  - CC:006C.01.01.11.003, If the requested operation is accepted but cannot be completed
 *    immediately, the <WORKING> status MUST be advertised in the Supervision Report.
 *    >Reason: Application specific.
 *  - CC:006C.01.01.11.00A, A receiving node MUST abort an active operation in favor of a new
 *    command with a new Session ID if that new command affects the same resources as the active operation
 *    >Reason: Framework and Sigma applications do always finish job.
 *  - CC:006C.01.01.13.001: A receiving node MAY abort an active operation in favor of a new command with
 *    a new Session ID even if the new command does not affects the same resources as the active operation,
 *    e.g. if the node does not have resources to handle multi-session state management
 *    >Reason: Framework and Sigma applications do always finish job.
 *  - CC:006C.01.02.11.004: If a Multi Command encapsulated group of commands is being supervised, an error
 *    indication (such as <No Support> or <Fail>) MUST be issued if just one of the Multi Command encapsulated
 *    commands triggers an error condition.
 *    >Reason: Framework do not handle Multi commands.
 *  - CC:006C.01.02.11.005: The <Success> indication MUST signify that all commands carried in a Multi Command
 *    encapsulation commands have completed successfully
 *    >Reason: Framework do not handle Multi commands.
 */
#include <string.h>
#include <ZW_TransportEndpoint.h>
#include <mock_control.h>
#include <ZW_transport_api.h>
#include "CC_Supervision.c"
#include <test_common.h>
#include "ZAF_CC_Invoker.h"
#include "cc_supervision_config_api_mock.h"
#include "cc_supervision_handlers_mock.h"
#include "zaf_transport_tx_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define handleCommandClassSupervision(a,b,c) invoke_cc_handler_v2(a,b,c,NULL,NULL)

extern uint8_t GetSuperVisionSessionID(void);
extern void SetSuperVisionSessionID(uint8_t value);
static void supervision_get_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    bool allowStatusUpdates,
    bool increaseSessionID,
    uint8_t * p_encapsulated_cmd,
    uint8_t encapsulated_cmd_length);

static uint8_t source_session_id = 0;

static const uint8_t SUPERVISION_GET_FRAME_SIZE = 4;
static const uint8_t SUPERVISION_REPORT_FRAME_SIZE = 5;
static const uint8_t BASIC_SET_V2_FRAME_SIZE = 3;

static const uint8_t SESSION_ID_TEST_VALUE = 23;
static uint8_t STATUS_TEST_VALUE = 0xff; // SUPERVISION_STATUS_SUCCESS
static const uint8_t DURATION_TEST_VALUE = 10;

static uint8_t howManyTimesWasGetHandled; // Remember to reset this before use.
static uint8_t howManyTimesWasReportHandled;

static RECEIVE_OPTIONS_TYPE_EX rxOpt;

void GetReceivedHandler(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs,
                        __attribute__((unused)) int cmock_num_calls)
{
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x20, pArgs->cmdClass, "Wrong command class");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x01, pArgs->cmd, "Wrong command");
  pArgs->duration = 20;
  pArgs->status = 0xff; // SUPERVISION_STATUS_SUCCESS
  TEST_ASSERT_EQUAL_PTR_MESSAGE(&rxOpt, pArgs->rxOpt, "rxOpt pointers doesn't match :(");
  howManyTimesWasGetHandled++;
}

void ReportReceivedHandler( cc_supervision_status_t status, uint8_t duration,
                            __attribute__((unused)) int cmock_num_calls)
{
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(STATUS_TEST_VALUE, status, "Wrong status value");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(DURATION_TEST_VALUE, duration, "Wrong duration value");
  howManyTimesWasReportHandled++;
}

void cc_supervision_get_received_handler_do_nothing(__attribute__((unused)) SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs,
                                                    __attribute__((unused)) int cmock_num_calls)
{
}

void cc_supervision_report_recived_handler_do_nothing(__attribute__((unused)) cc_supervision_status_t status,
                                                      __attribute__((unused)) uint8_t duration,
                                                      __attribute__((unused)) int cmock_num_calls)
{
}

void InitSupervisionCC(bool statusUpdates,
 CMOCK_cc_supervision_get_received_handler_CALLBACK getReceivedHandler,
 CMOCK_cc_supervision_report_recived_handler_CALLBACK reportReceivedHandler)
{
  cc_supervision_get_default_status_updates_enabled_IgnoreAndReturn(statusUpdates);
  if (NON_NULL(getReceivedHandler)) {
    // act as the origininal weak function is redefined by the given getReceivedHandler
    cc_supervision_get_received_handler_StubWithCallback(getReceivedHandler);
  } else {
    // act as the origininal weak function on cc_supervision_config_api.c
    cc_supervision_get_received_handler_StubWithCallback(cc_supervision_get_received_handler_do_nothing);
  }

  if (NON_NULL(reportReceivedHandler)) {
    // act as the origininal weak function is redefined by the given reportReceivedHandler
    cc_supervision_report_recived_handler_StubWithCallback(reportReceivedHandler);
  } else {
    // act as the origininal weak function on cc_supervision_config_api.c
    cc_supervision_report_recived_handler_StubWithCallback(cc_supervision_report_recived_handler_do_nothing);
  }
  
  // Init Supervision CC via ZAF
  ZAF_CC_init_specific(COMMAND_CLASS_SUPERVISION);
}



/**
 * CC:006C.01.01.11.001, CC:006C.01.01.11.002, CC:006C.01.01.11.00B, CC:006C.01.01.11.00C,
 * CC:006C.01.01.13.002, CC:006C.01.02.11.001, CC:006C.01.02.11.003:
 * This tests the reception of a Supervision Get with a Basic Set command.
 * 1. Single-cast:
 *    a. Transport_ApplicationCommandHandlerEx() is called by checking previously_receive_session_id.
 *    b. single-cast trigger supervision_report is send back.
 */
void test_receptionOfSupervisionGetSinglecast(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = 0; // Singlecast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  
  InitSupervisionCC(false, GetReceivedHandler, NULL);

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{

    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = false;

    zaf_transport_rx_to_tx_options_Expect(&rxOpt, NULL);
    zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

    uint8_t expected_frame[] = {0x6C, 0x02, sessionId, 0xff, 20};

    zaf_transport_tx_ExpectAndReturn(expected_frame, SUPERVISION_REPORT_FRAME_SIZE, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);
  }while(sessionId < runs);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sessionId, howManyTimesWasGetHandled, "Error number of runs");

  mock_calls_verify();
}

/**
 * CC:006C.01.01.11.006
 * CC:006C.01.01.11.009, A receiving node MUST ignore duplicate singlecast commands having the same Session ID
 * This tests the reception of a Supervision Get with a Basic Set command.
 * 1. Single-cast:
 *    a. Transport_ApplicationCommandHandlerEx() is called by checking previously_receive_session_id.
 *    b. single-cast trigger supervision_report is send back.
 */
void test_receptionOfSupervisionGetSinglecast_no_CommandClassSupervisionInit(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  //previously_receive_session_id = 0;
  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = 0; // Singlecast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  InitSupervisionCC(false, NULL, NULL);

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{
    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = false;

    zaf_transport_rx_to_tx_options_Expect(&rxOpt, NULL);
    zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

    uint8_t expected_frame[] = {0x6C, 0x02, sessionId, 0xff, 0}; // CC:006C.01.01.11.006 check sessionId

    zaf_transport_tx_ExpectAndReturn(expected_frame, SUPERVISION_REPORT_FRAME_SIZE, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    /*Second single-cast discarded!*/
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  }while(sessionId < runs);

  mock_calls_verify();
}


/**
 * CC:006C.01.01.13.003, CC:006C.01.01.11.004, CC:006C.01.01.11.005
 * 1. Multi-cast:
 *    a. If multi-cast is received (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
 *    b. Transport_ApplicationCommandHandlerEx() is called
 *    c. Do not send supervision_report.
 *
 * 2. Multi-cast single-cast follow up:
 *    a. Transport_ApplicationCommandHandlerEx is discarded on single-cast by checking previously_receive_session_id.
 *    b. single-cast trigger supervision_report is send back.
 */
void test_receptionOfSupervisionGetMulticastAndSinglecast(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI; // Multicast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  InitSupervisionCC(false, GetReceivedHandler, NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{
    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
    rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI; // Multicast.

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    // Call the command handler first time - with multicast.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    rxOpt.rxStatus = 0x00; // Singlecast.

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = false;

    zaf_transport_rx_to_tx_options_Expect(&rxOpt, NULL);
    zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

    uint8_t expected_frame[] = {0x6C, 0x02, sessionId, 0xff, 20};

    zaf_transport_tx_ExpectAndReturn(expected_frame, SUPERVISION_REPORT_FRAME_SIZE, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    // Call the command handler second time - now with singlecast.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(sessionId, howManyTimesWasGetHandled, "Get handler wasn't called :(");

  }
  while(sessionId < runs);

  mock_calls_verify();
}

/**
 * 1. Multi-cast:
 *    a. If multi-cast is received (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
 *    b. Transport_ApplicationCommandHandlerEx() is called
 *    c. Do not send supervision_report.
 *
 * 2. Multi-cast single-cast follow up:
 *    a. Transport_ApplicationCommandHandlerEx is discarded on single-cast by checking previously_receive_session_id.
 *    b. single-cast trigger supervision_report is send back.
 */
void test_receptionOfSupervisionGetMulticastAndSinglecast_no_CommandClassSupervisionInit(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  //previously_receive_session_id = 0;
  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI; // Multicast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  InitSupervisionCC(false, NULL, NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{
    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
    rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI; // Multicast.

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    // Call the command handler first time - with multicast.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    rxOpt.rxStatus = 0x00; // Singlecast.

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = false;

    zaf_transport_rx_to_tx_options_Expect(&rxOpt, NULL);
    zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

    uint8_t expected_frame[] = {0x6C, 0x02, sessionId, 0xff, 0};

    zaf_transport_tx_ExpectAndReturn(expected_frame, SUPERVISION_REPORT_FRAME_SIZE, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    // Call the command handler second time - now with singlecast.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  }while(sessionId < runs);

  mock_calls_verify();
}



/**
 * Verify the behavior when receiving a Multi Channel Bit addressing encapsulated singlecast frame.
 *
 * Supervision is expected to
 * - process the encapsulated CC by invoking Transport_ApplicationCommandHandlerEx(),
 * - invoke the get handler initialized in Supervision CC,
 * but not
 * - respond with a Supervision Report because Check_not_legal_response_job() returns true.
 */
void test_receptionOfSupervisionGetBitAdressing(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 1;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = 0; // A bit addressing frame is a singlecast frame.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  InitSupervisionCC(false, GetReceivedHandler, NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{
    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);

    // Setting BitAddress does not affect the test, but indicates how RX options actually would be.
    rxOpt.destNode.BitAddress = 1;
    rxOpt.destNode.endpoint = 1;
    rxOpt.destNode.nodeId = 2;

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = true;

    // Call the command handler first time - with bit addr endpoint 1.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2*sessionId-1, howManyTimesWasGetHandled, "Get handler was called :(");

    // Setting BitAddress does not affect the test, but indicates how RX options actually would be.
    rxOpt.destNode.BitAddress = 1;
    rxOpt.destNode.endpoint = 2;
    rxOpt.destNode.nodeId = 2;

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->return_code.v = true;

    // Call the command handler first time - with bit addr endpoint 2
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2*sessionId, howManyTimesWasGetHandled, "Get handler was called :(");

  }while(sessionId < runs);

  mock_calls_verify();
}

/**
 * Test: Single-cast CC multichannel bit-adr.:
 *    CommandClassMultiChan handle bit addressing by calling each endpoint with the payload.
 *    a. If Single-cast CC multichannel bit-adr. (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
 *    b. Transport_ApplicationCommandHandlerEx() must be called every time. Check previously_received_destination
 *       differ from EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.sessionid)
 *    c. Do not send supervision_report.
 */
void test_receptionOfSupervisionGetBitAdressing_no_CommandClassSupervisionInit(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  //previously_receive_session_id = 0;
  howManyTimesWasGetHandled = 0;

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 1;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI; // Multicast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  // Setup frame.
  uint8_t sessionId = 0;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = BASIC_SET_V2_FRAME_SIZE; // Size of "Basic Set v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + BASIC_SET_V2_FRAME_SIZE;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = 0x01; // Basic command set
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 2) = 0xFF; // Basic value "turn on".

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  InitSupervisionCC(false, NULL, NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  int runs = 10;
  do{
    sessionId++;
    pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
    rxOpt.destNode.BitAddress = 0;
    rxOpt.destNode.endpoint = 1;
    rxOpt.destNode.nodeId = 2;

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    // Call the command handler first time - with bit addr endpoint 1.
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    rxOpt.rxStatus = RECEIVE_STATUS_TYPE_MULTI;
    rxOpt.destNode.BitAddress = 0;
    rxOpt.destNode.endpoint = 2;
    rxOpt.destNode.nodeId = 2;

    mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
    pMock->expect_arg[0].p = &rxOpt;
    pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
    pMock->expect_arg[2].v = BASIC_SET_V2_FRAME_SIZE;
    pMock->return_code.v = 0xFF;  //CC_SUPERVISION_STATUS_SUCCESS;

    // Call the command handler first time - with bit addr endpoint 2
    handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, howManyTimesWasGetHandled, "Get handler wasn't called :(");

  }while(sessionId < runs);

  mock_calls_verify();
}

/**
 * CC:006C.01.02.11.006: Status values
 */
void test_SupervisionReceptionOfReport(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  // Setup receive options.
  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = 0x00; // Singlecast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  // Setup frame.
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_REPORT_FRAME * pCmd = (ZW_SUPERVISION_REPORT_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_REPORT;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(SESSION_ID_TEST_VALUE);

  pCmd->status = STATUS_TEST_VALUE;
  pCmd->duration = DURATION_TEST_VALUE;
  uint8_t cmdLength = SUPERVISION_REPORT_FRAME_SIZE;

  mock_t * pMock = NULL;

  /*STATUS_TEST_VALUE = 0xff; // SUPERVISION_STATUS_SUCCESS*/
  mock_call_expect(TO_STR(ZW_TransportMulticast_clearTimeout), &pMock);
  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  InitSupervisionCC(false, NULL, ReportReceivedHandler);

  SetSuperVisionSessionID( SESSION_ID_TEST_VALUE);

  handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, howManyTimesWasReportHandled, "Report handler wasn't called :(");


  /*CC_SUPERVISION_STATUS_WORKING*/
  STATUS_TEST_VALUE = 0x01; //CC_SUPERVISION_STATUS_WORKING
  pCmd->status = STATUS_TEST_VALUE;
  mock_call_expect(TO_STR(ZW_TransportMulticast_clearTimeout), &pMock);

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(false, NULL, ReportReceivedHandler);

  SetSuperVisionSessionID( SESSION_ID_TEST_VALUE);

  handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, howManyTimesWasReportHandled, "Report handler wasn't called :(");


  /*CC_SUPERVISION_STATUS_FAIL*/
  STATUS_TEST_VALUE = 0x02; //CC_SUPERVISION_STATUS_FAIL
  pCmd->status = STATUS_TEST_VALUE;
  mock_call_expect(TO_STR(ZW_TransportMulticast_clearTimeout), &pMock);

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(false, NULL, ReportReceivedHandler);

  SetSuperVisionSessionID( SESSION_ID_TEST_VALUE);

  handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, howManyTimesWasReportHandled, "Report handler wasn't called :(");

  /*CC_SUPERVISION_STATUS_NOT_SUPPORTED*/
  STATUS_TEST_VALUE = 0x00; //CC_SUPERVISION_STATUS_NOT_SUPPORTED
  pCmd->status = STATUS_TEST_VALUE;
  mock_call_expect(TO_STR(ZW_TransportMulticast_clearTimeout), &pMock);

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(false, NULL, ReportReceivedHandler);

  SetSuperVisionSessionID( SESSION_ID_TEST_VALUE);

  handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, howManyTimesWasReportHandled, "Report handler wasn't called :(");

  mock_calls_verify();
}

/**
 * CC:006C.01.02.11.002: verify More Status Updates.
 * CC:006C.01.02.11.003: Session Id MUST carry the same value as the Session ID field
 *    of the Supervision Get Command which initiated this session
 * CC:006C.01.02.11.012: test duration that it can send a byte from application.
 */
void test_CmdClassSupervisionReportSend(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  zaf_tx_options_t tx_options;
  uint8_t  properties, status, duration;

  tx_options.bit_addressing = false;
  tx_options.dest_endpoint = 0;
  tx_options.dest_node_id = 2;
  tx_options.security_key = 0;
  
  properties = CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(0) | CC_SUPERVISION_ADD_SESSION_ID(0x3F);
  status = CC_SUPERVISION_STATUS_SUCCESS;
  duration = 0xff;

  uint8_t expectedFrame[] = {
      COMMAND_CLASS_SUPERVISION,
      SUPERVISION_REPORT,
      0x3F,
      status,
      duration
  };

  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  TEST_ASSERT_TRUE_MESSAGE(
      CmdClassSupervisionReportSend(&tx_options, properties, status, duration ),
      "Command job CmdClassSupervisionReportSend failed");

  properties = CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(1) | CC_SUPERVISION_ADD_SESSION_ID(1);
  status = CC_SUPERVISION_STATUS_SUCCESS;
  duration = 0x0;

  uint8_t expectedFrame2[] = {
      COMMAND_CLASS_SUPERVISION,
      SUPERVISION_REPORT,
      0x81,
      status,
      duration
  };

  zaf_transport_tx_ExpectAndReturn(expectedFrame2, sizeof(expectedFrame2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  TEST_ASSERT_TRUE_MESSAGE(
      CmdClassSupervisionReportSend(&tx_options, properties, status, duration ),
      "Command job CmdClassSupervisionReportSend failed");

  mock_calls_verify();
}

/**
 * CC:006C.01.00.13.001 :The Supervision Command Class MAY be used for solitary commands
 * such as Set and unsolicited Report commands
 * CC:006C.01.01.11.006: Reserved field. This field MUST be set to 0 by a sending node
 * CC:006C.01.01.11.007: CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED
 * CC:006C.01.01.11.008: Increment session Id
 * Test adding Supervision Get on payload and check sessionId wrapping.
 */
void test_CC_SupervisionGet_Payload(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  uint8_t runs = 1;
  uint8_t sessionId = 1;

  //previously_receive_session_id = 0;
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );
  TEST_ASSERT_TRUE_MESSAGE(pTxBuf , "Error get response-buffer");

  SetSuperVisionSessionID(sessionId);
  m_status_updates_enabled = false;
  m_CommandLength = 0;
  do
  {

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(GetSuperVisionSessionID(), sessionId , "error m_sessionId pre");

    CommandClassSupervisionGetAdd((ZW_SUPERVISION_GET_FRAME*) pTxBuf);

    if(++sessionId == 0x40)
    {
      sessionId = 1;
      SetSuperVisionSessionID(sessionId);
    }

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(GetSuperVisionSessionID(), sessionId, "error m_sessionId post");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_status_updates_enabled, false, "error m_status_updates_enabled");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_CommandLength, 0, "error m_CommandLength");

    CommandClassSupervisionGetSetPayloadLength((ZW_SUPERVISION_GET_FRAME*) pTxBuf, runs);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_CommandLength, runs, "error m_CommandLength after changed");

    uint8_t length = CommandClassSupervisionGetGetPayloadLength((ZW_SUPERVISION_GET_FRAME*) pTxBuf);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(length , runs, "error m_CommandLength after changed" );
  }while(runs++ != 0xff);
}


/**
 * CC:006C.01.00.13.001 :The Supervision Command Class MAY be used for solitary commands
 * such as Set and unsolicited Report commands
 * CC:006C.01.01.11.006: Reserved field. This field MUST be set to 0 by a sending node
 * CC:006C.01.01.11.007: CC_SUPERVISION_STATUS_UPDATES_SUPPORTED
 * Test adding Supervision Get on payload and check sessionId wrapping.
 */
void test_CC_SupervisionGet_Payload_CC_SUPERVISION_STATUS_UPDATES_SUPPORTED(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  uint8_t runs = 1;
  uint8_t sessionId = 1;

  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );
  TEST_ASSERT_TRUE_MESSAGE(pTxBuf , "Error get response-buffer");

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(true, NULL, NULL);
  SetSuperVisionSessionID(sessionId);
  m_status_updates_enabled = true;
  m_CommandLength = 0;
  do
  {

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(GetSuperVisionSessionID(), sessionId , "error m_sessionId pre");

    CommandClassSupervisionGetAdd((ZW_SUPERVISION_GET_FRAME*) pTxBuf);

    if(++sessionId == 0x40)
    {
      sessionId = 1;
      SetSuperVisionSessionID(sessionId);
    }

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(GetSuperVisionSessionID(), sessionId, "error m_sessionId post");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_status_updates_enabled, 1, "error m_status_updates_enabled");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_CommandLength, 0, "error m_CommandLength");

    CommandClassSupervisionGetSetPayloadLength((ZW_SUPERVISION_GET_FRAME*) pTxBuf, runs);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(m_CommandLength, runs, "error m_CommandLength after changed");

    uint8_t length = CommandClassSupervisionGetGetPayloadLength((ZW_SUPERVISION_GET_FRAME*) pTxBuf);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(length , runs, "error m_CommandLength after changed" );
  }while(runs++ != 0xff);
}

/**
 * CC:006C.01.00.13.002
 * The Supervision Command Class MUST NOT be used for session-like command flows such as Get <-> Report
 *  command exchanges or firmware update
 */
void test_CC_Get__Report_Supervision_fail_report(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  // Setup receive options.
  memset((uint8_t *)&rxOpt, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.destNode.BitAddress = 0;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.rxStatus = 0; // Singlecast.
  rxOpt.securityKey = SECURITY_KEY_NONE;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.res = 0;

  mock_t * pMock = NULL;

  // Setup frame.
  uint8_t sessionId = 1;
  ZW_APPLICATION_TX_BUFFER frame;
  ZW_SUPERVISION_GET_FRAME * pCmd = (ZW_SUPERVISION_GET_FRAME *)&frame;
  pCmd->cmdClass = COMMAND_CLASS_SUPERVISION;
  pCmd->cmd = SUPERVISION_GET;
  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);
  pCmd->encapsulatedCommandLength = sizeof(ZW_BASIC_GET_FRAME); // Size of "Basic Get v2" frame.
  uint8_t cmdLength = SUPERVISION_GET_FRAME_SIZE + pCmd->encapsulatedCommandLength;
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 0) = 0x20; // Basic command class
  *((uint8_t*)pCmd + SUPERVISION_GET_FRAME_SIZE + 1) = BASIC_GET; // Basic command Get

  pCmd->properties1 = CC_SUPERVISION_ADD_SESSION_ID(sessionId);

  mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
  pMock->expect_arg[0].p = &rxOpt;
  pMock->expect_arg[1].p = (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + SUPERVISION_GET_FRAME_SIZE);
  pMock->expect_arg[2].v = 2;
  pMock->return_code.v = RECEIVED_FRAME_STATUS_FAIL;

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOpt;
  pMock->return_code.v = false;

  zaf_transport_rx_to_tx_options_Expect(&rxOpt, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  uint8_t expected_frame[] = {0x6C, 0x02, sessionId, CC_SUPERVISION_STATUS_FAIL, 00};

  zaf_transport_tx_ExpectAndReturn(expected_frame, SUPERVISION_REPORT_FRAME_SIZE, NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handleCommandClassSupervision(&rxOpt, (ZW_APPLICATION_TX_BUFFER *)pCmd, cmdLength);
  mock_calls_verify();
}

void test_session_id_increasing(void)
{
  SetSuperVisionSessionID(0);
  m_status_updates_enabled = 0;

  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * p_frame = (uint8_t *)&frame;

  for (uint8_t i = 0; i < 10; i++)
  {
    memset(p_frame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
    CommandClassSupervisionGetAdd((ZW_SUPERVISION_GET_FRAME *)&frame);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(i, *(p_frame + 2), "Session ID field is incorrect");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(i + 1, GetSuperVisionSessionID(), "Session ID variable is not incremented!");
  }
}

void printf_frame(uint8_t * p_frame, uint8_t frame_length)
{
  for (uint8_t i = 0; i < frame_length; i++)
  {
    printf("%#02x ", *(p_frame + i));
  }
}

void test_SUPERVISION_GET_ignore_identical_session_ids(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  typedef struct
  {
    bool increaseSessionID;
    received_frame_status_t expected_frame_status;
  }
  test_SUPERVISION_GET_ignore_identical_session_ids_test_vector_t;

  const test_SUPERVISION_GET_ignore_identical_session_ids_test_vector_t TEST_VECTOR[] = {
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = false, .expected_frame_status = RECEIVED_FRAME_STATUS_FAIL},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.increaseSessionID = false, .expected_frame_status = RECEIVED_FRAME_STATUS_FAIL},
    {.increaseSessionID = true, .expected_frame_status = RECEIVED_FRAME_STATUS_SUCCESS}
  };

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(false, NULL, NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  for (uint8_t i = 0; i < sizeof(TEST_VECTOR) / sizeof(test_SUPERVISION_GET_ignore_identical_session_ids_test_vector_t); i++)
  {
    printf("\nIteration %d: ", i);

    rxOptions.destNode.nodeId = 1;

    uint8_t encapsulated_cmd[] = {0xAA, 0xBB, 0xCC};
    uint8_t encapsulated_cmd_length = sizeof(encapsulated_cmd);

    supervision_get_frame_create(
        pFrame,
        &commandLength,
        true,
        TEST_VECTOR[i].increaseSessionID,
        encapsulated_cmd,
        encapsulated_cmd_length);

    printf_frame(pFrame, commandLength);

    /*
     * The following check is made because Supervision must handle encapsulated commands only if
     * the session ID change.
     */
    if (true == TEST_VECTOR[i].increaseSessionID)
    {
      mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_ANY;
      pMock->compare_rule_arg[1] = COMPARE_ANY;
      pMock->compare_rule_arg[2] = COMPARE_ANY;

      mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
      pMock->expect_arg[0].p = &rxOptions;
      pMock->return_code.v = false;

      zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
      zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
      zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

      zaf_transport_tx_ExpectAndReturn(NULL, 0, NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_frame();
      zaf_transport_tx_IgnoreArg_frame_length();
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();
    }

    received_frame_status_t status;
    status = handleCommandClassSupervision(&rxOptions, &frame, commandLength);
    TEST_ASSERT_MESSAGE(TEST_VECTOR[i].expected_frame_status == status, "Wrong receive status :(");
  }

  mock_calls_verify();
}

void test_SUPERVISION_GET_no_response_to_broadcast(void)
{
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  command_handler_input_t command_handler_input;
  test_common_clear_command_handler_input(&command_handler_input);

  command_handler_input.rxOptions.rxStatus = RECEIVE_STATUS_TYPE_BROAD;

  uint8_t encapsulated_cmd[] = {0xAA, 0xBB, 0xCC};
  uint8_t encapsulated_cmd_length = sizeof(encapsulated_cmd);

  supervision_get_frame_create(
      command_handler_input.frame.as_byte_array,
      &command_handler_input.frameLength,
      true,
      true,
      encapsulated_cmd,
      encapsulated_cmd_length);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));
  mock_call_use_as_stub(TO_STR(Transport_ApplicationCommandHandlerEx));

  received_frame_status_t status;
  status = handleCommandClassSupervision(
      &command_handler_input.rxOptions,
      &command_handler_input.frame.as_zw_application_tx_buffer,
      command_handler_input.frameLength);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == status, "Wrong receive status :(");

  mock_calls_verify();
}

void test_SUPERVISION_GET_no_response_to_followup_after_broadcast(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

#define FRAME_COUNT (4)

  command_handler_input_t command_handler_input[FRAME_COUNT];

  test_common_clear_command_handler_input_array(command_handler_input, FRAME_COUNT);

  typedef struct
  {
    struct
    {
      bool increaseSessionID;
      uint8_t rxStatus;
    }
    input;
    struct
    {
      bool response;
      received_frame_status_t received_frame_status;
    }
    expect;
  }
  test_vector_t;

  const test_vector_t TEST_VECTOR[FRAME_COUNT] = {
    // Supervision Get number 1 with a new session ID
    {.input.increaseSessionID = true, .input.rxStatus = RECEIVE_STATUS_TYPE_BROAD, .expect.response = false, .expect.received_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    {.input.increaseSessionID = false, .input.rxStatus = RECEIVE_STATUS_TYPE_SINGLE, .expect.response = true, .expect.received_frame_status = RECEIVED_FRAME_STATUS_SUCCESS},
    // Supervision Get number 2 with an unchanged session ID
    {.input.increaseSessionID = false, .input.rxStatus = RECEIVE_STATUS_TYPE_BROAD, .expect.response = false, .expect.received_frame_status = RECEIVED_FRAME_STATUS_FAIL},
    {.input.increaseSessionID = false, .input.rxStatus = RECEIVE_STATUS_TYPE_SINGLE, .expect.response = false, .expect.received_frame_status = RECEIVED_FRAME_STATUS_FAIL}
  };

  for (uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    command_handler_input[i].rxOptions.rxStatus = TEST_VECTOR[i].input.rxStatus;
  }

  source_session_id = 0;

  uint8_t encapsulated_cmd[] = {0xAA, 0xBB, 0xCC};
  uint8_t encapsulated_cmd_length = sizeof(encapsulated_cmd);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));
  mock_call_use_as_stub(TO_STR(Transport_ApplicationCommandHandlerEx));

  for (uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    printf("\nIteration %d: ", i);

    supervision_get_frame_create(
        command_handler_input[i].frame.as_byte_array,
        &command_handler_input[i].frameLength,
        false,
        TEST_VECTOR[i].input.increaseSessionID,
        encapsulated_cmd,
        encapsulated_cmd_length);

    printf_frame(command_handler_input[i].frame.as_byte_array, command_handler_input[i].frameLength);

    if (true == TEST_VECTOR[i].expect.response)
    {
      mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_ANY;
      pMock->return_code.v = false;

      zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
      zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
      zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

      zaf_transport_tx_ExpectAndReturn(NULL, 0, NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_frame();
      zaf_transport_tx_IgnoreArg_frame_length();
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();
    }

    received_frame_status_t status;
    status = handleCommandClassSupervision(
        &command_handler_input[i].rxOptions,
        &command_handler_input[i].frame.as_zw_application_tx_buffer,
        command_handler_input[i].frameLength);

    TEST_ASSERT_MESSAGE(TEST_VECTOR[i].expect.received_frame_status == status, "Wrong receive status :(");
  }

  mock_calls_verify();
}

void SUPERVISION_GET_command_handler_status_fail_get_handler( SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs,
                                                              __attribute__((unused)) int cmock_num_calls)
{
  /*
   * The status must be set to something else than CC_SUPERVISION_STATUS_FAIL because we're
   * verifying that this function is never called and the status returned by the Supervision Report
   * is the status of the Application Command Handler (CC_SUPERVISION_STATUS_FAIL).
   */
  pArgs->status = CC_SUPERVISION_STATUS_WORKING;
}

/**
 * Verifies that the Supervision Get handler is NOT called if the Application Command Handler
 * returns FAIL on a specific command meaning the command could not be executed.
 */
void test_SUPERVISION_GET_command_handler_status_fail(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  command_handler_input_t chi_supervision_get;
  test_common_clear_command_handler_input(&chi_supervision_get);

  uint8_t encapsulated_cmd[] = {0xAA, 0xBB, 0xCC};
  uint8_t encapsulated_cmd_length = sizeof(encapsulated_cmd);

  source_session_id = 60; // Random session ID.

  supervision_get_frame_create(
      chi_supervision_get.frame.as_byte_array,
      &chi_supervision_get.frameLength,
      false,
      true,
      encapsulated_cmd,
      encapsulated_cmd_length);

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(
      false,
      SUPERVISION_GET_command_handler_status_fail_get_handler,
      NULL);

  mock_call_use_as_stub(TO_STR(ZAF_CC_MultiChannel_IsCCSupported));

  mock_call_expect(TO_STR(Transport_ApplicationCommandHandlerEx), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->compare_rule_arg[2] = COMPARE_ANY;
  pMock->return_code.v = RECEIVED_FRAME_STATUS_FAIL;

  mock_call_use_as_stub(TO_STR(GetResponseBuffer));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->return_code.v = false;

  zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  const uint8_t EXPECTED_FRAME[] = {
      COMMAND_CLASS_SUPERVISION,
      SUPERVISION_REPORT,
      source_session_id,
      2, // Status = FAIL
      0 // Duration
  };
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  status = handleCommandClassSupervision(
      &chi_supervision_get.rxOptions,
      &chi_supervision_get.frame.as_zw_application_tx_buffer,
      chi_supervision_get.frameLength);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == status, "Wrong receive status :(");

  mock_calls_verify();
}

static bool SUPERVISION_GET_cc_not_supported_was_called;

void SUPERVISION_GET_cc_not_supported(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs,
                                      __attribute__((unused)) int cmock_num_calls)
{
  SUPERVISION_GET_cc_not_supported_was_called = true;
}

/**
 * According to requirement CC:006C.01.00.21.001 in SDS13783-4, Supervision CC must be present in
 * a nodes NIF "regardless of the inclusion status and security bootstrapping outcome."
 *
 * In order for a secure command (e.g. Door Lock Operation set) not to be handled if it is
 * transmitted inside a non-secure Supervision Get, a check is added.
 *
 * This test verifies that if the check fails, Supervision will not call the Application Command
 * Handler.
 */
void test_SUPERVISION_GET_cc_not_supported(void)
{
  mock_t * pMock = NULL;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_get_pseudo_random));

  command_handler_input_t chi_supervision_get;
  test_common_clear_command_handler_input(&chi_supervision_get);

  uint8_t encapsulated_cmd[] = {0xAA, 0xBB, 0xCC};
  uint8_t encapsulated_cmd_length = sizeof(encapsulated_cmd);

  SetSuperVisionSessionID(0);
  source_session_id = 10; // Random session ID.

  supervision_get_frame_create(
      chi_supervision_get.frame.as_byte_array,
      &chi_supervision_get.frameLength,
      false,
      true,
      encapsulated_cmd,
      encapsulated_cmd_length);

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));
  InitSupervisionCC(
      false,
      SUPERVISION_GET_cc_not_supported,
      NULL);

  mock_call_expect(TO_STR(ZAF_CC_MultiChannel_IsCCSupported), &pMock);
  pMock->expect_arg[0].p = &chi_supervision_get.rxOptions;
  pMock->expect_arg[1].p = chi_supervision_get.frame.as_byte_array + sizeof(ZW_SUPERVISION_GET_FRAME);
  pMock->return_code.v = false;

  zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->return_code.v = false;

  const uint8_t EXPECTED_FRAME[] = {
      COMMAND_CLASS_SUPERVISION,
      SUPERVISION_REPORT,
      source_session_id,
      CC_SUPERVISION_STATUS_NOT_SUPPORTED,
      0 // Duration
  };
  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_callback();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t status;
  status = handleCommandClassSupervision(
      &chi_supervision_get.rxOptions,
      &chi_supervision_get.frame.as_zw_application_tx_buffer,
      chi_supervision_get.frameLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == status, "Wrong receive status :(");
  TEST_ASSERT_FALSE_MESSAGE(SUPERVISION_GET_cc_not_supported_was_called, "The Supervision Get handler was called!");

  mock_calls_verify();
}

void test_cc_supervision_status_t(void)
{
  TEST_ASSERT(0 == CC_SUPERVISION_STATUS_NOT_SUPPORTED);
  TEST_ASSERT(1 == CC_SUPERVISION_STATUS_WORKING);
  TEST_ASSERT(2 == CC_SUPERVISION_STATUS_FAIL);
  TEST_ASSERT(0xFF == CC_SUPERVISION_STATUS_SUCCESS);
}

/**
 * Creates a SUPERVISION_GET frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 */
static void supervision_get_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    bool allowStatusUpdates,
    bool increaseSessionID,
    uint8_t * p_encapsulated_cmd,
    uint8_t encapsulated_cmd_length)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_SUPERVISION;
  pFrame[frameCount++] = SUPERVISION_GET;
  if (true == increaseSessionID)
  {
    source_session_id++;
  }
  pFrame[frameCount++] = ((((allowStatusUpdates == true) ? 1 : 0) << 7) & 0x80) | (source_session_id & 0x3F);
  pFrame[frameCount++] = encapsulated_cmd_length;
  memcpy(pFrame + 4, p_encapsulated_cmd, encapsulated_cmd_length);
  *pFrameLength = frameCount + encapsulated_cmd_length;
}
