/**
 * @file
 *
 * @copyright 2022 Silicon Laboratories Inc.
 *
 */
#include <zaf_event_distributor.h>
#include <zaf_event_distributor_soc.h>
#include <ZAF_ApplicationEvents.h>      // for EApplicationEvent
#include <ev_man.h>                     // for EVENT_SYSTEM
#include <queue_mock.h>
#include <task_mock.h>
#include <ZAF_Common_interface_mock.h>
#include <ZAF_Common_helper_mock.h>
#include <ZAF_network_learn_mock.h>
#include <QueueNotifying_mock.h>
#include <zaf_job_helper_mock.h>
#include <EventDistributor_mock.h>
#include <board_indicator_mock.h>
#include <ZW_TransportSecProtocol_mock.h>
#include <ZAF_CmdPublisher_mock.h>
#include <ZAF_nvm_mock.h>
#include <ZAF_nvm_app_mock.h>
#include <zaf_nvm_soc_mock.h>
#include <zpal_misc_mock.h>
#include <CC_DeviceResetLocally_mock.h>
#include <zpal_watchdog_mock.h>
#include <CC_Indicator_mock.h>

static QueueHandle_t queue_handle = { 0 };
static TaskHandle_t task_handle = { 0 };
static uint32_t notification_pending;

static EEventDistributorStatus
EventDistributorConfig_callback(SEventDistributor* pThis,
                                uint8_t iEventHandlerTableSize,
                                const EventDistributorEventHandler* pEventHandlerTable,
                                cmock_EventDistributor_func_ptr1 pNoEvent,
                                int cmock_num_calls)
{
  pThis->iEventHandlerTableSize = iEventHandlerTableSize;
  pThis->pEventHandlerTable = pEventHandlerTable;
  pThis->pNoEvent = pNoEvent;

  return EEVENTDISTRIBUTOR_STATUS_SUCCESS;
}

static uint32_t
EventDistributorDistribute_callback(const SEventDistributor* pThis,
                                    uint32_t iEventWait,
                                    uint32_t NotificationClearMask,
                                    int cmock_num_calls)
{
  pThis->pEventHandlerTable[notification_pending]();

  return 0;
}

static void
CC_DeviceResetLocally_notification_tx_callback(int cmock_num_calls)
{
  TRANSMISSION_RESULT transmission_result;
  SApplicationHandles app_handles;
  SZwaveCommandPackage command_package;

  transmission_result.isFinished = TRANSMISSION_RESULT_FINISHED;
  app_handles.pZwCommandQueue = (SQueueNotifying*)0x02;
  command_package.eCommandType = EZWAVECOMMANDTYPE_SET_DEFAULT;

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);
  zpal_enable_watchdog_Expect(false);
  QueueNotifyingSendToBack_ExpectAndReturn(app_handles.pZwCommandQueue, (uint8_t*) &command_package, 500, EQUEUENOTIFYING_STATUS_SUCCESS);
  CC_DeviceResetLocally_done(&transmission_result);
}

void
zaf_event_distributor_app_event_manager(const uint8_t event)
{
}

void
setUpSuite(void)
{
}

void
tearDownSuite(void)
{
}

void
setUp(void)
{
  xQueueGenericCreateStatic_IgnoreAndReturn(queue_handle);
  ZAF_getAppTaskHandle_IgnoreAndReturn(task_handle);
  QueueNotifyingInit_Ignore();
  ZAF_JobHelperInit_Ignore();

  EventDistributorConfig_StubWithCallback(EventDistributorConfig_callback);

  zafi_nvm_app_load_configuration_Ignore();

  zaf_event_distributor_init();
}

void
tearDown(void)
{
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_EMPTY
 *
 */
void test_initialization_and_use_application_event(void)
{
  uint8_t event;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_EMPTY;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_START
 * InclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_learn_mode_start_excluded(void)
{
  uint8_t event;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_LEARNMODE_START;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_INCLUSION);

  Board_IndicateStatus_Expect(BOARD_STATUS_LEARNMODE_ACTIVE);

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_START
 * InclusionState: EINCLUSIONSTATE_SECURE_INCLUDED
 */
void test_learn_mode_start_included(void)
{
  uint8_t event;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_SECURE_INCLUDED;
  app_handles.pNetworkInfo = &network_info;
  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_LEARNMODE_START;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);

  Board_IndicateStatus_Expect(BOARD_STATUS_LEARNMODE_ACTIVE);

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_STOP
 */
void test_learn_mode_stop(void)
{
  uint8_t event;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_LEARNMODE_STOP;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_DISABLE);

  Board_IndicateStatus_Expect(BOARD_STATUS_IDLE);

  Board_IsIndicatorActive_ExpectAndReturn(false);
  CC_Indicator_RefreshIndicatorProperties_Expect();

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_FINISHED
 */
void test_learn_mode_finished(void)
{
  uint8_t event;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_LEARNMODE_FINISHED;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  Board_IndicateStatus_Expect(BOARD_STATUS_IDLE);

  Board_IsIndicatorActive_ExpectAndReturn(false);
  CC_Indicator_RefreshIndicatorProperties_Expect();

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_learn_mode_finished_Expect();

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_TOGGLE
 */
void test_learn_mode_toggle_1(void)
{
  uint8_t event, event_2;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_LEARNMODE_TOGGLE;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  event_2 = EVENT_SYSTEM_LEARNMODE_START;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event_2, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_LEARNMODE_START -> EVENT_SYSTEM_LEARNMODE_TOGGLE
 * InclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_learn_mode_toggle_2(void)
{
  uint8_t event_1, event_2, event_3;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event_1 = EVENT_SYSTEM_LEARNMODE_START;
  xQueueReceive_ExpectAndReturn(queue_handle, &event_1, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_1, sizeof(uint8_t));

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  ZAF_setNetworkLearnMode_Expect(E_NETWORK_LEARN_MODE_INCLUSION);

  Board_IndicateStatus_Expect(BOARD_STATUS_LEARNMODE_ACTIVE);

  event_2 = EVENT_SYSTEM_LEARNMODE_TOGGLE;
  xQueueReceive_ExpectAndReturn(queue_handle, &event_2, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_2, sizeof(uint8_t));

  event_3 = EVENT_SYSTEM_LEARNMODE_STOP;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event_3, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(queue_handle, &event_2, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_2, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_FLUSHMEM_READY
 */
void test_app_event_manager_flushmem_ready_1(void)
{
  uint8_t event;

  notification_pending = EAPPLICATIONEVENT_APP;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event = EVENT_SYSTEM_FLUSHMEM_READY;
  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zafi_nvm_app_reset_Expect();

  zafi_nvm_app_load_configuration_Expect();

  xQueueReceive_ExpectAndReturn(queue_handle, &event, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test App Event Manager
 * EVENT: EVENT_SYSTEM_RESET -> EVENT_SYSTEM_FLUSHMEM_READY
 */
void test_app_event_manager_flushmem_ready_2(void)
{
  uint8_t event_1, event_2;

  notification_pending = EAPPLICATIONEVENT_APP;
  const uint16_t MFGID_ZWAVE_ALLIANCE = 0x031D;
  const uint16_t RESET_INFO = 0x0004;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  event_1 = EVENT_SYSTEM_RESET;
  xQueueReceive_ExpectAndReturn(queue_handle, &event_1, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_1, sizeof(uint8_t));

  CC_DeviceResetLocally_notification_tx_StubWithCallback(CC_DeviceResetLocally_notification_tx_callback);

  event_2 = EVENT_SYSTEM_FLUSHMEM_READY;
  xQueueReceive_ExpectAndReturn(queue_handle, &event_2, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_2, sizeof(uint8_t));

  zafi_nvm_app_reset_Expect();

  zpal_reboot_with_info_Expect(MFGID_ZWAVE_ALLIANCE, RESET_INFO);

  xQueueReceive_ExpectAndReturn(queue_handle, &event_2, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_2, sizeof(uint8_t));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_UNUSED1
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_UNUSED1;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_TX
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * TxStatus.bIsTxFrameLegal = true
 * TxStatus.Handle = NULL
 */
void test_initialization_and_use_zw_command_status_tx_1(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_TX;
  Status.Content.TxStatus.bIsTxFrameLegal = true;
  Status.Content.TxStatus.Handle = NULL;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_TX
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * TxStatus.bIsTxFrameLegal = false
 */
void test_initialization_and_use_zw_command_status_tx_2(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_TX;
  Status.Content.TxStatus.bIsTxFrameLegal = false;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

static uint8_t zw_command_status_tx_handle_counter = 0;

static void
zw_command_status_tx_handle(void)
{
  zw_command_status_tx_handle_counter++;
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_TX
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * TxStatus.bIsTxFrameLegal = true
 * TxStatus.Handle = zw_command_status_tx_handle
 */
void test_initialization_and_use_zw_command_status_tx_3(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;
  zw_command_status_tx_handle_counter = 0;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_TX;
  Status.Content.TxStatus.bIsTxFrameLegal = true;
  Status.Content.TxStatus.Handle = zw_command_status_tx_handle;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_stay_awake_Expect();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();

  TEST_ASSERT_EQUAL_INT8(1, zw_command_status_tx_handle_counter);
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_TX
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * TxStatus.bIsTxFrameLegal = false
 * TxStatus.Handle = zw_command_status_tx_handle
 */
void test_initialization_and_use_zw_command_status_tx_4(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;
  zw_command_status_tx_handle_counter = 0;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_TX;
  Status.Content.TxStatus.bIsTxFrameLegal = false;
  Status.Content.TxStatus.Handle = zw_command_status_tx_handle;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();

  TEST_ASSERT_EQUAL_INT8(0, zw_command_status_tx_handle_counter);
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_GENERATE_RANDOM
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_generate_random(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_GENERATE_RANDOM;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * SecurityKeys: 0
 * LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_COMPLETE
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_1(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  network_info.SecurityKeys = 0;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_COMPLETE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zafi_nvm_app_set_default_configuration_Expect();

  event = EVENT_SYSTEM_LEARNMODE_FINISHED;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  ZAF_Transport_OnLearnCompleted_Expect();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_NODEID_DONE
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_2(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_NODEID_DONE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_RANGE_INFO_UPDATE
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_3(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_RANGE_INFO_UPDATE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_INFO_PENDING
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_4(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_INFO_PENDING;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_WAITING_FOR_FIND
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_5(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_WAITING_FOR_FIND;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_SMART_START_IN_PROGRESS
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_6(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_SMART_START_IN_PROGRESS;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  event = EVENT_SYSTEM_SMARTSTART_IN_PROGRESS;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_LEARN_IN_PROGRESS
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_7(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_LEARN_IN_PROGRESS;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_8(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  event = EVENT_SYSTEM_LEARNMODE_FINISHED;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.LearnModeStatus.Status: ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_9(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  event = EVENT_SYSTEM_RESET;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS
 * eInclusionState: EINCLUSIONSTATE_SECURE_INCLUDED
 * SecurityKeys: SECURITY_KEY_S2_UNAUTHENTICATED_BIT
 * Content.LearnModeStatus.Status: ELEARNSTATUS_ASSIGN_COMPLETE
 */
void test_initialization_and_use_zw_command_status_learn_mode_status_10(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_SECURE_INCLUDED;
  network_info.SecurityKeys = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS;
  Status.Content.LearnModeStatus.Status = ELEARNSTATUS_ASSIGN_COMPLETE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  GetHighestSecureLevel_ExpectAndReturn(network_info.SecurityKeys, SECURITY_KEY_S2_AUTHENTICATED);

  zafi_nvm_app_set_default_configuration_Expect();

  event = EVENT_SYSTEM_LEARNMODE_FINISHED;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  ZAF_Transport_OnLearnCompleted_Expect();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_NETWORK_LEARN_MODE_START
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_network_learn_mode_start(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_NETWORK_LEARN_MODE_START;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_SET_DEFAULT
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_set_default(void)
{
  uint8_t event;
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_SET_DEFAULT;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  event = EVENT_SYSTEM_FLUSHMEM_READY;
  QueueNotifyingSendToBack_ExpectAndReturn(NULL, &event, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_INVALID_TX_REQUEST
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_invalid_tx_request(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_INVALID_TX_REQUEST;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_INVALID_COMMAND
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_invalid_command(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_INVALID_COMMAND;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_ZW_SET_MAX_INCL_REQ_INTERVALS
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_command_status_zw_set_max_incl_req_intervals(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_MAX_INCL_REQ_INTERVALS;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_ZW_SET_TX_ATTENUATION
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.SetTxAttenuation.result: true
 */
void test_initialization_and_use_zw_command_status_zw_set_tx_attenuation_1(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_TX_ATTENUATION;
  Status.Content.SetTxAttenuation.result = true;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW Command Status
 * eStatusType: EZWAVECOMMANDSTATUS_ZW_SET_TX_ATTENUATION
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 * Content.SetTxAttenuation.result: false
 */
void test_initialization_and_use_zw_command_status_zw_set_tx_attenuation_2(void)
{
  SZwaveCommandStatusPackage Status;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwCommandStatusQueue = (QueueHandle_t)0x06;
  notification_pending = EAPPLICATIONEVENT_ZWCOMMANDSTATUS;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  Status.eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_TX_ATTENUATION;
  Status.Content.SetTxAttenuation.result = false;

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  xQueueReceive_ExpectAndReturn(app_handles.ZwCommandStatusQueue, &Status, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&Status, sizeof(Status));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW RX
 * eReceiveType: EZWAVERECEIVETYPE_SINGLE
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_rx_single(void)
{
  SZwaveReceivePackage RxPackage;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;
  CP_Handle_t cp_handle;

  cp_handle = (CP_Handle_t)0x05;
  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwRxQueue = (QueueHandle_t)0x04;
  notification_pending = EAPPLICATIONEVENT_ZWRX;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  RxPackage.eReceiveType = EZWAVERECEIVETYPE_SINGLE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  ZAF_getCPHandle_ExpectAndReturn(cp_handle);
  ZAF_CP_CommandPublish_Expect(cp_handle, &RxPackage);

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW RX
 * eReceiveType: EZWAVERECEIVETYPE_NODE_UPDATE
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_rx_node_update(void)
{
  SZwaveReceivePackage RxPackage;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwRxQueue = (QueueHandle_t)0x04;
  notification_pending = EAPPLICATIONEVENT_ZWRX;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  RxPackage.eReceiveType = EZWAVERECEIVETYPE_NODE_UPDATE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_stay_awake_Expect();

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW RX
 * eReceiveType: EZWAVERECEIVETYPE_SECURITY_EVENT
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_rx_security_event(void)
{
  SZwaveReceivePackage RxPackage;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwRxQueue = (QueueHandle_t)0x04;
  notification_pending = EAPPLICATIONEVENT_ZWRX;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  RxPackage.eReceiveType = EZWAVERECEIVETYPE_SECURITY_EVENT;

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_event_distributor_distribute();
}

/**
 * Test ZW RX
 * eReceiveType: EZWAVERECEIVETYPE_STAY_AWAKE
 * eInclusionState: EINCLUSIONSTATE_EXCLUDED
 */
void test_initialization_and_use_zw_rx_stay_awake(void)
{
  SZwaveReceivePackage RxPackage;
  SApplicationHandles app_handles;
  SNetworkInfo network_info;

  network_info.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
  app_handles.pNetworkInfo = &network_info;
  app_handles.ZwRxQueue = (QueueHandle_t)0x04;
  notification_pending = EAPPLICATIONEVENT_ZWRX;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  ZAF_getAppHandle_ExpectAndReturn(&app_handles);

  RxPackage.eReceiveType = EZWAVERECEIVETYPE_STAY_AWAKE;

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_stay_awake_Expect();

  xQueueReceive_ExpectAndReturn(app_handles.ZwRxQueue, &RxPackage, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&RxPackage, sizeof(RxPackage));

  zaf_event_distributor_distribute();
}

static void
handler_uint8_type(const uint8_t event, const void *uint8)
{
  uint8_t data;

  data = *(uint8_t*)uint8;

  TEST_ASSERT_EQUAL_INT8(0x01, event);
  TEST_ASSERT_EQUAL_INT8(0x05, data);
}

ZAF_EVENT_DISTRIBUTOR_REGISTER_CC_EVENT_HANDLER(0x01, handler_uint8_type);

/**
 * Test CC Event Manager
 * CC: 0x01
 * EVENT: 0x01
 *
 */
void test_initialization_and_use_cc_event(void)
{
  static uint8_t data = 0x05;
  event_cc_t event_cc = {
    .command_class = 0x01,
    .event = 0x01,
    .data = &data
  };

  notification_pending = EAPPLICATIONEVENT_CC;

  EventDistributorDistribute_StubWithCallback(EventDistributorDistribute_callback);

  xQueueReceive_ExpectAndReturn(queue_handle, &event_cc, 0, pdTRUE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_cc, sizeof(event_cc_t));

  xQueueReceive_ExpectAndReturn(queue_handle, &event_cc, 0, pdFALSE);
  xQueueReceive_IgnoreArg_pvBuffer(); // Used as output
  xQueueReceive_ReturnMemThruPtr_pvBuffer(&event_cc, sizeof(event_cc_t));

  zaf_event_distributor_distribute();
}

/**
 * Test CC Event enqueue
 *
 */
void test_zaf_event_distributor_enqueue_cc_event(void)
{
  bool ret;
  static uint8_t data = 0x05;
  event_cc_t event_cc = {
    .command_class = 0x01,
    .event = 0x01,
    .data = &data
  };

  QueueNotifyingSendToBack_ExpectAndReturn(NULL, (uint8_t*) &event_cc, 0, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  ret = zaf_event_distributor_enqueue_cc_event(event_cc.command_class, event_cc.event, event_cc.data);
  TEST_ASSERT_TRUE(ret);
}

/**
 * Test CC Event enqueue fail
 *
 */
void test_zaf_event_distributor_enqueue_cc_event_fail(void)
{
  bool ret;
  static uint8_t data = 0x05;
  event_cc_t event_cc = {
    .command_class = 0x01,
    .event = 0x01,
    .data = &data
  };

  QueueNotifyingSendToBack_ExpectAndReturn(NULL, (uint8_t*) &event_cc, 0, EQUEUENOTIFYING_STATUS_TIMEOUT);
  QueueNotifyingSendToBack_IgnoreArg_pThis();

  ret = zaf_event_distributor_enqueue_cc_event(event_cc.command_class, event_cc.event, event_cc.data);
  TEST_ASSERT_FALSE(ret);
}

/**
 * Test CC Event enqueue from ISR
 *
 */
void test_zaf_event_distributor_enqueue_cc_event_from_isr(void)
{
  bool ret;
  static uint8_t data = 0x05;
  event_cc_t event_cc = {
    .command_class = 0x01,
    .event = 0x01,
    .data = &data
  };

  QueueNotifyingSendToBackFromISR_ExpectAndReturn(NULL, (uint8_t*) &event_cc, EQUEUENOTIFYING_STATUS_SUCCESS);
  QueueNotifyingSendToBackFromISR_IgnoreArg_pThis();

  ret = zaf_event_distributor_enqueue_cc_event_from_isr(event_cc.command_class, event_cc.event, event_cc.data);
  TEST_ASSERT_TRUE(ret);
}

/**
 * Test CC Event enqueue from ISR fail
 *
 */
void test_zaf_event_distributor_enqueue_cc_event_from_isr_fail(void)
{
  bool ret;
  static uint8_t data = 0x05;
  event_cc_t event_cc = {
    .command_class = 0x01,
    .event = 0x01,
    .data = &data
  };

  QueueNotifyingSendToBackFromISR_ExpectAndReturn(NULL, (uint8_t*) &event_cc, EQUEUENOTIFYING_STATUS_TIMEOUT);
  QueueNotifyingSendToBackFromISR_IgnoreArg_pThis();

  ret = zaf_event_distributor_enqueue_cc_event_from_isr(event_cc.command_class, event_cc.event, event_cc.data);
  TEST_ASSERT_FALSE(ret);
}
