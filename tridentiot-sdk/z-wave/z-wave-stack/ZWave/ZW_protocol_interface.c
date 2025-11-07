// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol_interface.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @startuml
 * title Application <-> ZwStack interface
 * box "Application task" #LightBlue
 * participant Application
 * end box
 * participant TxQueue
 * participant CommandQueue
 * participant StatusQueue
 * participant RxQueue
 * box "ZwStack task" #Pink
 * participant ZwStack
 * end box
 * participant Radio
 * group Receiver task context
 * == Tx ==
 *   [->Application: Reason to send frame
 *   activate Application
 *   Application->TxQueue: Put frame on queue
 *   activate ZwStack
 *   TxQueue<->ZwStack: Take frame off queue
 *   note left: OS context switches to ZwStack
 *   ZwStack->Radio: Tx Frame
 *   deactivate ZwStack
 *   activate Radio
 *   TxQueue->Application: Application continues\nfrom QueueSendToback()
 *   note left: OS context swithces to Application
 *   [<-Application: Finishes
 *   deactivate Application
 *   ...All tasks are sleeping...
 *   Radio->ZwStack: Tx Complete
 *   deactivate Radio
 *   activate ZwStack
 *   ZwStack->Radio: Listen for ack
 *   deactivate ZwStack
 *   activate Radio
 *   ...All tasks are sleeping...
 *   Radio->ZwStack: Ack received
 *   deactivate Radio
 *   activate ZwStack
 *   ZwStack->StatusQueue: Put 'Frame successfully sent' on queue
 *   deactivate ZwStack
 *   activate Application
 *   StatusQueue<->Application: Take status off queue
 *   note left: Application awakened
 *   [<-Application: Finished processing status
 *   deactivate Application
 * == Command ==
 *   [->Application: Reason to send command
 *   activate Application
 *   Application->CommandQueue: Put command on queue
 *   activate ZwStack
 *   CommandQueue<->ZwStack: Take command off queue
 *   note left: OS context switches to ZwStack
 *   ...ZwStack processes command...
 *   ZwStack->StatusQueue: Put 'Command status' on queue
 *   deactivate ZwStack
 *   StatusQueue->Application: Take status off queue
 *   note left: OS context switches to Application
 *   [<-Application: Finished processing status
 *   deactivate Application
 * == Rx ==
 *   Radio->ZwStack: Frame received
 *   note right: ZwStack awakened
 *   activate ZwStack
 *   ZwStack->RxQueue: Put frame on queue
 *   deactivate ZwStack
 *   activate Application
 *   RxQueue<->Application: Take frame off queue
 *   note left: Application awakened
 *   ...Application thread processes frame...
 * @enduml
 */

#include "ZW_classcmd.h"
#include "ZW_lib_defines.h"
#include "ZW_protocol_interface.h"


/* FreeeRTOS includes*/
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
// Generic includes
#include <string.h>
#include <stddef.h>

/* Components includes*/
#include "QueueNotifying.h"
//#define DEBUGPRINT
#include "DebugPrint.h"

#include "Assert.h"
#include "SizeOf.h"

/* Z-Wave includes */
#include "ZW_application_transport_interface.h"
#include "ZW_transport_api.h"
#include "ZW_transport.h"
#include "ZW_basis.h"
#include "ZW_network_management.h"
#include "ZW_main.h"
#ifdef ZW_CONTROLLER
#include "ZW_initiate_shutdown.h"
#endif
#include "ZW_noise_detect.h"
#include <ZW.h>
#ifdef ZW_CONTROLLER
#include "ZW_timer.h"
#include "ZW_controller.h"
#include "ZW_controller_network_info_storage.h"
#else   // #ifdef ZW_CONTROLLER
#include "ZW_slave.h"
#endif  // #ifdef ZW_CONTROLLER
#include "zpal_entropy.h"
#include <ZW_main_region.h>

#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_Security_Scheme0.h>
#include <ZW_Security_Scheme2.h>
#include <ZW_s2_inclusion_glue.h>
#include <ZW_timer.h>
#endif

#ifdef USE_TRANSPORT_SERVICE
#include <transport_service2_external.h>
#endif

#include <ZW_explore.h>
#include <ZW_lr_virtual_node_id.h>

#include <zpal_radio.h>
#include <zpal_bootloader.h>
#include <zpal_nvm.h>
#include <zpal_misc.h>

#include <ZW_tx_queue.h>

/* The TSE can queue up multiple frames at once, with 2 end points and
2 lifeline destinations it is immediately 4 frames to send.
MAXIMUM_FRAMES_IN_APPLICATION_TX_QUEUE should be high enough so that lifeline status are reported correctly */
#define MAXIMUM_FRAMES_IN_APPLICATION_TX_QUEUE 5

#define PM_STAY_AWAKE_DURATION_3_MIN        (1000 * 60 * 3)

// Application <-> Zwave protocol application queues
static SQueueNotifying m_TxNotifyingQueue = { 0 };            // Queue with Tx requests from app to protocol
static StaticQueue_t m_TxQueueObject = { 0 };
static QueueHandle_t m_TxQueueHandle;
static SZwaveTransmitPackage m_aTxQueueStorage[MAXIMUM_FRAMES_IN_APPLICATION_TX_QUEUE] = {{ 0 }};

static SQueueNotifying m_CommandStatusNotifyingQueue = { 0 }; // Queue with command status from Protocol to app
static StaticQueue_t m_CommandStatusQueueObject = { 0 };
static QueueHandle_t m_CommandStatusQueueHandle;
static SZwaveCommandStatusPackage m_aCommandStatusQueueStorage[4] = {{ 0 }};

static SQueueNotifying m_RxNotifyingQueue = { 0 };            // Queue with Rx frames from protocol to app
static StaticQueue_t m_RxQueueObject = { 0 };
static QueueHandle_t m_RxQueueHandle;
static SZwaveReceivePackage m_aRxQueueStorage[2] = {{ 0 }};

static SQueueNotifying m_CommandNotifyingQueue = { 0 };       // Queue with commands
static StaticQueue_t m_CommandQueueObject = { 0 };
static QueueHandle_t m_CommandQueueHandle;
static SZwaveCommandPackage m_aCommandQueueStorage[2] = {{ 0 }};


static uint8_t m_iFramesInProtocol;     /**< Number of frames currently
                                             being processed by protocol */
static uint8_t m_AppTxHandlerEventBitNumber;  /**< FreeRtos Task notification bit that activates
                                                   ProtocolInterfaceAppTxHandler   */

static uint8_t m_nwl = false;         /**< Flag telling if we are sending network wide inclusion\exclusion */

static const SApplicationInterface * m_pAppInterface;

#define ELIBRARYTYPE_CONTROLLER_TEST  13

static const SProtocolInfo m_ProtocolInfo = {
#ifdef ZW_SECURITY_PROTOCOL
                                              .CommandClassVersions.SecurityVersion = TRANSPORT_SECURITY_SUPPORTED_VERSION,
                                              .CommandClassVersions.Security2Version = TRANSPORT_SECURITY_2_SUPPORTED_VERSION,
#else // #ifdef ZW_SECURITY_PROTOCOL
                                              .CommandClassVersions.SecurityVersion = UNKNOWN_VERSION,
                                              .CommandClassVersions.Security2Version = UNKNOWN_VERSION,
#endif //#ifdef ZW_SECURITY_PROTOCOL

#ifdef USE_TRANSPORT_SERVICE
                                              .CommandClassVersions.TransportServiceVersion = TRANSPORT_SERVICE2_SUPPORTED_VERSION,
#else // #ifdef USE_TRANSPORT_SERVICE
                                              .CommandClassVersions.TransportServiceVersion = UNKNOWN_VERSION,
#endif // #ifdef USE_TRANSPORT_SERVICE

                                              .ProtocolVersion.Major = ZW_VERSION_MAJOR,
                                              .ProtocolVersion.Minor = ZW_VERSION_MINOR,
                                              .ProtocolVersion.Revision = ZW_VERSION_PATCH,
                                              .eProtocolType = EPROTOCOLTYPE_ZWAVE,

#ifdef ZW_CONTROLLER
#ifdef ZW_CONTROLLER_BRIDGE
                                              .eLibraryType =  ELIBRARYTYPE_CONTROLLER
#else
                                              .eLibraryType =  ELIBRARYTYPE_CONTROLLER_PORTABLE
#endif
#elif defined(ZW_SLAVE)
                                              .eLibraryType =  ELIBRARYTYPE_SLAVE
#endif
                                            };

static SNetworkInfo m_NetworkInfo = { .SucNodeId = 0,   // SucNodeId must be initialized to zero
                                                        // As that is its default init value in protocol
                                      .eInclusionState = EINCLUSIONSTATE_EXCLUDED,
                                      .SecurityKeys = 0,
                                      .MaxPayloadSize = 0
                                    };

static SLongRangeInfo m_LongRangeInfo = { .MaxLongRangePayloadSize = 0};

static SRadioStatus m_RadioStatus = { 0 };

#ifdef ZW_SECURITY_PROTOCOL
SSwTimer delayAppTxHandlerEventTimer;
static void delayAppTxHandlerEventTimeout(SSwTimer *timer);
#endif

static bool ProtocolInterfacePassToAppFrame(const SZwaveReceivePackage* pReceivePackage);
static void ZCB_SendDataCallback(ZW_Void_Function_t Context, uint8_t status, TX_STATUS_TYPE* pTxStatusReport);
static bool AddStatusToCommandStatusQueue(SZwaveCommandStatusPackage* pStatusPackage);
static void ZCB_ExplorerRequestClusion(ZW_Void_Function_t Context, uint8_t TxStatus, TX_STATUS_TYPE* pTxStatusReport);
static void InclusionStateUpdate(void);
static void MaxPayloadSizeUpdate(void);
#ifdef ZW_CONTROLLER
static void ZCB_LearnModeStatus(LEARN_INFO_T*  pLearnStatus);
#endif // #ifdef ZW_CONTROLLER
#ifdef ZW_SLAVE
static void ZCB_LearnModeStatus(ELearnStatus Status, node_id_t NodeId);
#endif // #ifdef ZW_SLAVE
#ifdef ZW_CONTROLLER
static void RemoveFailedNodeIDCB(uint8_t bStatus)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_REMOVE_FAILED_NODE_ID,
    .Content.FailedNodeIDStatus.result = bStatus
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

static void ReplaceFailedNodeIDCB(uint8_t bStatus)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_REPLACE_FAILED_NODE_ID,
    .Content.FailedNodeIDStatus.result = bStatus
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

#endif

void ProtocolInterfaceInit(
                            SApplicationInterface* pAppInterface,
                            uint8_t AppTxHandlerEventBitNumber,
                            uint8_t AppCommandHandlerEventBitNumber
                          )
{
  m_AppTxHandlerEventBitNumber = AppTxHandlerEventBitNumber;
  m_iFramesInProtocol = 0;
  m_nwl = false;

  pAppInterface->AppHandles.pProtocolInfo = &m_ProtocolInfo;

  // Setup application read address for network info
  pAppInterface->AppHandles.pNetworkInfo = &m_NetworkInfo;

  // Setup application read address for Long Range info
  pAppInterface->AppHandles.pLongRangeInfo = &m_LongRangeInfo;

  // Setup application read address for radio status
  pAppInterface->AppHandles.pRadioStatus = &m_RadioStatus;

  /*
   * Initialize the max payload size here. Z-Wave must be initialized before this point because
   * ZW_GetMaxPayloadSize() depends on RF region.
   */
  MaxPayloadSizeUpdate();

  // Tx Queue
  m_TxQueueHandle = xQueueCreateStatic(
    sizeof_array(m_aTxQueueStorage),
    sizeof(m_aTxQueueStorage[0]),
    (uint8_t*)m_aTxQueueStorage,
    &m_TxQueueObject
  );

  TaskHandle_t TaskHandle = xTaskGetCurrentTaskHandle();

  QueueNotifyingInit(
    &m_TxNotifyingQueue,
    m_TxQueueHandle,
    TaskHandle,
    AppTxHandlerEventBitNumber
  );

  pAppInterface->AppHandles.pZwTxQueue = &m_TxNotifyingQueue;

  // Command queue
  m_CommandQueueHandle = xQueueCreateStatic(
    sizeof_array(m_aCommandQueueStorage),
    sizeof(m_aCommandQueueStorage[0]),
    (uint8_t*)m_aCommandQueueStorage,
    &m_CommandQueueObject
  );

  QueueNotifyingInit(
    &m_CommandNotifyingQueue,
    m_CommandQueueHandle,
    TaskHandle,
    AppCommandHandlerEventBitNumber
  );

  pAppInterface->AppHandles.pZwCommandQueue = &m_CommandNotifyingQueue;

  // Command status queue
  m_CommandStatusQueueHandle = xQueueCreateStatic(
    sizeof_array(m_aCommandStatusQueueStorage),
    sizeof(m_aCommandStatusQueueStorage[0]),
    (uint8_t*)m_aCommandStatusQueueStorage,
    &m_CommandStatusQueueObject
  );

  QueueNotifyingInit(
    &m_CommandStatusNotifyingQueue,
    m_CommandStatusQueueHandle,
    pAppInterface->ApplicationTaskHandle,
    pAppInterface->iZwCommandStatusQueueTaskNotificationBitNumber
  );


  pAppInterface->AppHandles.ZwCommandStatusQueue = m_CommandStatusQueueHandle;

  // Rx Queue
  m_RxQueueHandle = xQueueCreateStatic(
    sizeof_array(m_aRxQueueStorage),
    sizeof(m_aRxQueueStorage[0]),
    (uint8_t*)m_aRxQueueStorage,
    &m_RxQueueObject
  );

  QueueNotifyingInit(
    &m_RxNotifyingQueue,
    m_RxQueueHandle,
    pAppInterface->ApplicationTaskHandle,
    pAppInterface->iZwRxQueueTaskNotificationBitNumber
  );

  pAppInterface->AppHandles.ZwRxQueue = m_RxQueueHandle;

#ifdef ZW_SECURITY_PROTOCOL
  ZwTimerRegister(&delayAppTxHandlerEventTimer, false, delayAppTxHandlerEventTimeout);
#endif
  m_pAppInterface = pAppInterface;
}


void ProtocolInterfaceReset(void)
{
  if (!m_nwl)
  {
    m_iFramesInProtocol = 0;
  }


  // Reset all the interface queues
  xQueueReset(m_pAppInterface->AppHandles.pZwTxQueue->Queue);       // Used to receive in coming Tx Commands from the app.
  xQueueReset(m_pAppInterface->AppHandles.ZwCommandStatusQueue);    // Used to send responses back to the app asynchronously.
  xQueueReset(m_pAppInterface->AppHandles.ZwRxQueue);
  xQueueReset(m_pAppInterface->AppHandles.pZwCommandQueue->Queue);  // Used to receive in coming Commands from the app.

  // Remaining items like SucNodeId, Powerlevel etc. is updated as Protocol code resetting the values
  // must call the Set methods (ProtocolInterfaceSucNodeIdSet) in the Protocol Interface.
}

static SZwaveReceivePackage ReceivePackage = { 0 };

#ifdef ZW_CONTROLLER_BRIDGE

bool ProtocolInterfacePassToAppMultiFrame(
                                          uint8_t NodeMaskOffset,
                                          uint8_t NodeMaskLength,
                                          const uint8_t* pNodeMask,
                                          const ZW_APPLICATION_TX_BUFFER *pCommand,
                                          uint8_t iCommandLength,
                                          const RECEIVE_OPTIONS_TYPE *rxOpt
                                        )
{

  if (iCommandLength > sizeof(ReceivePackage.uReceiveParams.RxMulti.Payload))
  {
    return false;
  }

  memset(&ReceivePackage, 0, sizeof(ReceivePackage)); // Do not remove memset, Nodemask part is expected zeroed.

  ReceivePackage.uReceiveParams.RxMulti.iCommandLength = iCommandLength;
  memcpy(&ReceivePackage.uReceiveParams.RxMulti.Payload, pCommand, iCommandLength);
  ReceivePackage.uReceiveParams.RxMulti.RxOptions = *rxOpt;

  // Copy the received nodemask into the correct position in a full size nodemask.
  // We leave the remaining destination nodemask alone, as its zeroed by earlier memset.
  memcpy(((uint8_t*)&ReceivePackage.uReceiveParams.RxMulti.NodeMask) + (4 * NodeMaskOffset), pNodeMask, NodeMaskLength);

  ReceivePackage.eReceiveType = EZWAVERECEIVETYPE_MULTI;
  return ProtocolInterfacePassToAppFrame(&ReceivePackage);
}

#else

bool ProtocolInterfacePassToAppSingleFrame(
                                            uint8_t iCommandLength,
                                            const ZW_APPLICATION_TX_BUFFER *pCommand,  // payload
                                            const RECEIVE_OPTIONS_TYPE *pRxOptions
                                          )
{
  if (iCommandLength > sizeof(ReceivePackage.uReceiveParams.Rx.Payload))
  {
    return false;
  }

  /**
   * All NIFs (Node-Info) must either only go to the app or to the unify module.
   * When having the unify module, the app is communicating with Z-Wave through the unify module.
   * (Such changes are only made for Z-Wave functions that the unify code is using/supporting. All other functions are left as-is.)
   */
  memset(&ReceivePackage, 0, sizeof(ReceivePackage));

  ReceivePackage.uReceiveParams.Rx.iLength = iCommandLength;
  memcpy(&ReceivePackage.uReceiveParams.Rx.Payload, pCommand, iCommandLength);
  ReceivePackage.uReceiveParams.Rx.RxOptions = *pRxOptions;

  ReceivePackage.eReceiveType = EZWAVERECEIVETYPE_SINGLE;
  return ProtocolInterfacePassToAppFrame(&ReceivePackage);
}
#endif  // NOT ZW_CONTROLLER_BRIDGE


bool ProtocolInterfacePassToAppNodeUpdate(
                                          uint8_t Status,
                                          node_id_t NodeId,
                                          const uint8_t *pCommand,
                                          uint8_t iLength
                                          )
{
  if (iLength > sizeof_structmember(SReceiveNodeUpdate, aPayload))
  {
    return false;
  }

  /**
   * All NIFs (Node-Info) must either only go to the app or to the unify module.
   * When having the unify module, the app is communicating with Z-Wave through the unify module.
   *
   * (Such changes are only made for Z-Wave functions that the unify code is using/supporting. All other functions are left as-is.)
   */
  memset(&ReceivePackage, 0, sizeof(ReceivePackage));
  ReceivePackage.uReceiveParams.RxNodeUpdate.NodeId = NodeId;
  ReceivePackage.uReceiveParams.RxNodeUpdate.Status = Status;
  ReceivePackage.uReceiveParams.RxNodeUpdate.iLength = iLength;
  memcpy(&ReceivePackage.uReceiveParams.RxNodeUpdate.aPayload, pCommand, iLength);


  ReceivePackage.eReceiveType = EZWAVERECEIVETYPE_NODE_UPDATE;
  return ProtocolInterfacePassToAppFrame(&ReceivePackage);
}

bool ProtocolInterfacePassToAppSecurityEvent(
                                              const s_application_security_event_data_t *pSecurityEvent
                                            )
{
  uint8_t iLength = pSecurityEvent->eventDataLength;
  if (iLength > sizeof_structmember(SReceiveSecurityEvent, aEventData))
  {
    return false;
  }

  memset(&ReceivePackage, 0, sizeof(ReceivePackage));

  ReceivePackage.uReceiveParams.RxSecurityEvent.iLength = iLength;
  memcpy(&ReceivePackage.uReceiveParams.RxSecurityEvent.aEventData, pSecurityEvent->eventData, iLength);
  ReceivePackage.uReceiveParams.RxSecurityEvent.Event = pSecurityEvent->event;

  ReceivePackage.eReceiveType = EZWAVERECEIVETYPE_SECURITY_EVENT;
  return ProtocolInterfacePassToAppFrame(&ReceivePackage);
}

bool ProtocolInterfacePassToAppStayAwake(void)
{
  memset(&ReceivePackage, 0, sizeof(ReceivePackage));
  ReceivePackage.eReceiveType = EZWAVERECEIVETYPE_STAY_AWAKE;
  return ProtocolInterfacePassToAppFrame(&ReceivePackage);
}

static bool ProtocolInterfacePassToAppFrame(const SZwaveReceivePackage* pReceivePackage)
{
  EQueueNotifyingStatus Status;

  Status = QueueNotifyingSendToBack(&m_RxNotifyingQueue, (const uint8_t*)pReceivePackage, 0);
  if (Status != EQUEUENOTIFYING_STATUS_SUCCESS)
  {
    DPRINT("Failed to pass frame to application\r\n");
  }

  return Status == EQUEUENOTIFYING_STATUS_SUCCESS;
}


static void ProtocolInterfaceAppTxEventSet(void)
{
  xTaskNotify(xTaskGetCurrentTaskHandle(), 1<<m_AppTxHandlerEventBitNumber, eSetBits);
}

void ProtocolInterfaceAppTxEventUpdate(void)
{
  if (0 != uxQueueMessagesWaiting(m_pAppInterface->AppHandles.pZwTxQueue->Queue))
  {
    // message queue not empty make sure AppTxEvent gets handled
    ProtocolInterfaceAppTxEventSet();
  }
}


#ifdef ZW_SECURITY_PROTOCOL
static void delayAppTxHandlerEventTimeout(__attribute__((unused)) SSwTimer *pTimer)
{
  ProtocolInterfaceAppTxEventUpdate();
}

bool ProtocolInterfaceIsSecurityBusy(security_key_t eKeyType)
{
  if (SECURITY_KEY_NONE == eKeyType)
  {
    return false;
  }
  if (SECURITY_KEY_S0 == eKeyType)
  {
    return sec0_busy();
  }
  return s2_busy();
}
#endif

void ProtocolInterfaceAppTxHandler(void)
{
  // we have to make a copy of the multicast frame payload as it will be retransmitted when sending the single cast frames
  static uint8_t mutliCastFramePayload[TX_BUFFER_SIZE];
  SZwaveTransmitPackage TransmitPackage = { 0 };
  SZwaveTransmitPackage* pTransmitPackage = &TransmitPackage;

#ifdef ZW_SECURITY_PROTOCOL
  if (true == TimerIsActive(&delayAppTxHandlerEventTimer))
  {
    TimerStop(&delayAppTxHandlerEventTimer);
  }
#endif

  // Peek frame at first, so it is still there if we are unable to transmit it right now
  while (xQueuePeek(m_pAppInterface->AppHandles.pZwTxQueue->Queue, (uint8_t*)pTransmitPackage, 0) == pdTRUE)
  {
    bool bStatus;
    bool bInvalidRequest = false;
    uint8_t bStatusOnFailure = 0xff;

    SZwaveCommandStatusPackage FailStatus = {
                                              .eStatusType = EZWAVECOMMANDSTATUS_TX,
                                              .Content.TxStatus.bIsTxFrameLegal = false,
                                              .Content.TxStatus.Handle = NULL,
                                              .Content.TxStatus.TxStatus = 0,
                                             };
    STransmitCallback TxCallback = { 0 };
    ZW_TransmitCallbackUnBind(&TxCallback);
    /*
      We always start with STransmitCallback variable that is initialized with NULL pointer so API without callback
      are handled correctly
    */
    DPRINTF("xQueue Handling %d\n", pTransmitPackage->eTransmitType);

    switch (pTransmitPackage->eTransmitType)
    {
    case EZWAVETRANSMITTYPE_EXPLOREINCLUSIONREQUEST:
      // NOTE: sending this while not in correct inclusion mode causes protocol to return bad status
      DPRINT("xQueue ExploreInclusionRequest\n");
      m_nwl = true;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_ExplorerRequestClusion, 0);
      bStatus = ZW_ExploreRequestInclusion(&TxCallback);
      break;

    case EZWAVETRANSMITTYPE_EXPLOREEXCLUSIONREQUEST:
      // NOTE - if you send this while not in correct inclusion mode, protocol will PROBABLY return bad status
      DPRINT("xQueue ExploreExclusionRequest\n");
      m_nwl = true;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_ExplorerRequestClusion, 0);
      bStatus = ZW_ExploreRequestExclusion(&TxCallback);
      break;

    case EZWAVETRANSMITTYPE_NETWORKUPDATEREQUEST:
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pTransmitPackage->uTransmitParams.NetworkUpdateRequest.Handle);
      bStatus = ZW_RequestNetWorkUpdate(&TxCallback);
      break;

    case EZWAVETRANSMITTYPE_NODEINFORMATION:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pTransmitPackage->uTransmitParams.NodeInfo.Handle);
      bStatus = ZW_SendNodeInformation(
        pTransmitPackage->uTransmitParams.NodeInfo.DestNodeId,
        pTransmitPackage->uTransmitParams.NodeInfo.TransmitOptions,
        &TxCallback
        );
      break;

    case EZWAVETRANSMITTYPE_NODEINFORMATIONREQUEST:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pTransmitPackage->uTransmitParams.NodeInfoRequest.Handle);
      bStatus = ZW_RequestNodeInfo(
        pTransmitPackage->uTransmitParams.NodeInfoRequest.DestNodeId,
        &TxCallback
        );

      break;

    case EZWAVETRANSMITTYPE_TESTFRAME:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.Test.Handle);
      bStatus = ZW_SendTestFrame(
        pTransmitPackage->uTransmitParams.Test.DestNodeId,
        pTransmitPackage->uTransmitParams.Test.PowerLevel,
        &TxCallback
        );
      break;

    case EZWAVETRANSMITTYPE_STD:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendData.FrameConfig.Handle);
      bStatus = ZW_SendData(
        pTransmitPackage->uTransmitParams.SendData.DestNodeId,
        &pTransmitPackage->uTransmitParams.SendData.FrameConfig.aFrame[0],
        pTransmitPackage->uTransmitParams.SendData.FrameConfig.iFrameLength,
        pTransmitPackage->uTransmitParams.SendData.FrameConfig.TransmitOptions,
        &TxCallback
        );
      break;

#ifdef ZW_SECURITY_PROTOCOL
    case EZWAVETRANSMITTYPE_EX:
    {
      // First check if Security needed and if specified security engine is Busy
      if (false == ProtocolInterfaceIsSecurityBusy(pTransmitPackage->uTransmitParams.SendDataEx.eKeyType))
      {
        bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
        ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendDataEx.FrameConfig.Handle);
        TRANSMIT_OPTIONS_TYPE TxOptions = {
          .destNode = pTransmitPackage->uTransmitParams.SendDataEx.DestNodeId,
          .bSrcNode = pTransmitPackage->uTransmitParams.SendDataEx.SourceNodeId,
          .txOptions = pTransmitPackage->uTransmitParams.SendDataEx.FrameConfig.TransmitOptions,
          .txSecOptions = pTransmitPackage->uTransmitParams.SendDataEx.TransmitSecurityOptions,
          .securityKey = pTransmitPackage->uTransmitParams.SendDataEx.eKeyType,
          .txOptions2 = pTransmitPackage->uTransmitParams.SendDataEx.TransmitOptions2
        };
        bStatus = ZW_SendDataEx(
          &pTransmitPackage->uTransmitParams.SendDataEx.FrameConfig.aFrame[0],
          pTransmitPackage->uTransmitParams.SendDataEx.FrameConfig.iFrameLength,
          &TxOptions,
          &TxCallback
          );
      }
      else
      {
         // Post event again delayed - security engine not ready to handle a new frame
        TimerStart(&delayAppTxHandlerEventTimer, 10);
        return;
      }
      break;
    }
#endif  // ifdef ZW_SECURITY_PROTOCOL

#ifdef ZW_CONTROLLER_BRIDGE
    case EZWAVETRANSMITTYPE_BRIDGE:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendDataBridge.FrameConfig.Handle);
      bStatus = ZW_SendData_Bridge(
        pTransmitPackage->uTransmitParams.SendDataBridge.SourceNodeId,
        pTransmitPackage->uTransmitParams.SendDataBridge.DestNodeId,
        &pTransmitPackage->uTransmitParams.SendDataBridge.FrameConfig.aFrame[0],
        pTransmitPackage->uTransmitParams.SendDataBridge.FrameConfig.iFrameLength,
        pTransmitPackage->uTransmitParams.SendDataBridge.FrameConfig.TransmitOptions,
        &TxCallback
        );
      break;
#endif  // ifdef ZW_CONTROLLER_BRIDGE
#ifndef ZW_CONTROLLER_BRIDGE
    case EZWAVETRANSMITTYPE_MULTI:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      memcpy(mutliCastFramePayload,
             pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.aFrame,
             pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.iFrameLength);
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.Handle);
      bStatus = ZW_SendDataMulti(
        (uint8_t*)&pTransmitPackage->uTransmitParams.SendDataMulti.NodeMask,
        mutliCastFramePayload,
        pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.iFrameLength,
        pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.TransmitOptions,
        &TxCallback
        );
      break;
#endif  // ifndef ZW_CONTROLLER_BRIDGE

#ifdef ZW_SECURITY_PROTOCOL
    case EZWAVETRANSMITTYPE_MULTI_EX:
    {
      // First check if Security needed and if specified security engine is Busy
      if (false == ProtocolInterfaceIsSecurityBusy(pTransmitPackage->uTransmitParams.SendDataMultiEx.eKeyType))
      {
        bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;

        memcpy(mutliCastFramePayload,
               pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.aFrame,
               pTransmitPackage->uTransmitParams.SendDataMulti.FrameConfig.iFrameLength);
        ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendDataMultiEx.FrameConfig.Handle);
        TRANSMIT_MULTI_OPTIONS_TYPE TransmitOptions = {
          .groupID = pTransmitPackage->uTransmitParams.SendDataMultiEx.GroupId,
          .bSrcNode = pTransmitPackage->uTransmitParams.SendDataMultiEx.SourceNodeId,
          .txOptions = pTransmitPackage->uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions,
          .securityKey = pTransmitPackage->uTransmitParams.SendDataMultiEx.eKeyType
        };
        bStatus = ZW_SendDataMultiEx(
          mutliCastFramePayload,
          pTransmitPackage->uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength,
          &TransmitOptions,
          &TxCallback
          );
      }
      else
      {
         // Post event again delayed - security engine not ready to handle a new frame
         TimerStart(&delayAppTxHandlerEventTimer, 10);
        return;
      }
      break;
    }
#endif  // ifdef ZW_SECURITY_PROTOCOL

#ifdef ZW_CONTROLLER_BRIDGE
    case EZWAVETRANSMITTYPE_MULTI_BRIDGE:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      memcpy(mutliCastFramePayload,
             pTransmitPackage->uTransmitParams.SendDataMultiBridge.FrameConfig.aFrame,
             pTransmitPackage->uTransmitParams.SendDataMultiBridge.FrameConfig.iFrameLength);
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, (ZW_Void_Function_t)pTransmitPackage->uTransmitParams.SendDataMultiBridge.FrameConfig.Handle);
      bStatus = ZW_SendDataMulti_Bridge(
          pTransmitPackage->uTransmitParams.SendDataMultiBridge.SourceNodeId,
          pTransmitPackage->uTransmitParams.SendDataMultiBridge.NodeMask,
          mutliCastFramePayload,
          pTransmitPackage->uTransmitParams.SendDataMultiBridge.FrameConfig.iFrameLength,
          pTransmitPackage->uTransmitParams.SendDataMultiBridge.FrameConfig.TransmitOptions,
          pTransmitPackage->uTransmitParams.SendDataMultiBridge.lr_nodeid_list,
          &TxCallback);
      break;
#endif  // ifdef ZW_CONTROLLER_BRIDGE

#ifdef ZW_CONTROLLER

    case EZWAVETRANSMITTYPE_SETSUCNODEID:
    {
      bStatusOnFailure = ZW_SUC_SET_FAILED;
      SSetSucNodeId* pSetSucNodeId = &pTransmitPackage->uTransmitParams.SetSucNodeId;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pSetSucNodeId->Handle);
      bStatus = ZW_SetSUCNodeID(
                                pSetSucNodeId->SucNodeId,
                                pSetSucNodeId->bSucEnable,
                                pSetSucNodeId->bTxLowPower,
                                pSetSucNodeId->Capabilities,
                                &TxCallback
                              );
      break;
    }

    case EZWAVETRANSMITTYPE_SENDSUCNODEID:
    {
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      SSendSucNodeId* pSendSucNodeId = &pTransmitPackage->uTransmitParams.SendSucNodeId;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pSendSucNodeId->Handle);
      bStatus = ZW_SendSUCID(
        pSendSucNodeId->DestNodeId,
        pSendSucNodeId->TransmitOptions,
        &TxCallback
        );
      break;
    }

    case EZWAVETRANSMITTYPE_ASSIGNRETURNROUTE:
    {
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      SAssignReturnRoute* pAssignReturnRoute = &pTransmitPackage->uTransmitParams.AssignReturnRoute;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pAssignReturnRoute->Handle);
      uint8_t aPriorityRoute[sizeof_array(pAssignReturnRoute->aPriorityRouteRepeaters) + 1];
      uint8_t* pPriorityRoute = NULL;
      if (pAssignReturnRoute->PriorityRouteSpeed != 0)
      {
        memcpy(aPriorityRoute,
               pAssignReturnRoute->aPriorityRouteRepeaters,
               sizeof_array(pAssignReturnRoute->aPriorityRouteRepeaters));
        aPriorityRoute[sizeof_array(pAssignReturnRoute->aPriorityRouteRepeaters)] = pAssignReturnRoute->PriorityRouteSpeed;
        pPriorityRoute = aPriorityRoute;
      }
      bStatus = ZW_AssignReturnRoute(
        pAssignReturnRoute->ReturnRouteReceiverNodeId,
        pAssignReturnRoute->RouteDestinationNodeId,
        pPriorityRoute,
        pAssignReturnRoute->isSucRoute,
        &TxCallback
        );
      break;
    }

    case EZWAVETRANSMITTYPE_DELETERETURNROUTE:
    {
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;

      SDeleteReturnRoute* pDeleteReturnRoute = &pTransmitPackage->uTransmitParams.DeleteReturnRoute;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pDeleteReturnRoute->Handle);
      bStatus = ZW_DeleteReturnRoute(
        pDeleteReturnRoute->DestNodeId,
        pDeleteReturnRoute->bDeleteSuc,
        &TxCallback
        );
      break;
    }
#ifdef ZW_CONTROLLER_BRIDGE
    case EZWAVETRANSMITTYPE_SEND_SLAVE_NODE_INFORMATION:
    {
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      SSendSlaveNodeInformation* pSendSlaveNodeInfo = &pTransmitPackage->uTransmitParams.SendSlaveNodeInformation;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pSendSlaveNodeInfo->Handle);
      bStatus = ZW_SendSlaveNodeInformation(pSendSlaveNodeInfo->sourceId,
                                            pSendSlaveNodeInfo->destinationId,
                                            pSendSlaveNodeInfo->txOptions,
                                            &TxCallback);
      break;
    }
#endif //ZW_CONTROLLER_BRIDGE
#endif // ZW_CONTROLLER

#ifdef ZW_SLAVE
    case EZWAVETRANSMITTYPE_REQUESTNEWROUTEDESTINATIONS:
    {
      bStatusOnFailure = ZW_ROUTE_UPDATE_ABORT;
      SRequestNewRouteDestinations* pRequestNewRouteDestinations = &pTransmitPackage->uTransmitParams.RequestNewRouteDestinations;
      ZW_TransmitCallbackBind(&TxCallback, ZCB_SendDataCallback, pRequestNewRouteDestinations->Handle);
      bStatus = ZW_RequestNewRouteDestinations(
        pRequestNewRouteDestinations->aNewDestinations,
        pRequestNewRouteDestinations->iDestinationCount,
        &TxCallback
        );
      break;
    }
#endif // ZW_SLAVE

    case EZWAVETRANSMITTYPE_INCLUDEDNODEINFORMATION:
      bStatusOnFailure = TRANSMIT_COMPLETE_FAIL;
      bStatus = ExploreINIF();
      break;
    default:
    {
      bInvalidRequest = true;
      break;
    }

    } // End switch

    if (bInvalidRequest)
    {
      SZwaveCommandStatusPackage UnknownTxStatus =
      {
        .eStatusType = EZWAVECOMMANDSTATUS_INVALID_TX_REQUEST,
        .Content.InvalidTxRequestStatus.InvalidTxRequest = pTransmitPackage->eTransmitType
      };
      DPRINTF("Error: Unknown ZwTxQueue Transmit type %d\r\n", pTransmitPackage->eTransmitType);
      AddStatusToCommandStatusQueue(&UnknownTxStatus);
    }
    else if(!bStatus) //  Valid Request, but status NOT OK
    {
      //Woops - Protocol wouldnt accept cmd of of type %d, illegal frame?, just busy, or a special handling
      // if the api return false then we map an appropriate error code.
      FailStatus.Content.TxStatus.Handle = TxCallback.Context;
      FailStatus.Content.TxStatus.bIsTxFrameLegal = true;
      FailStatus.Content.TxStatus.TxStatus = bStatusOnFailure;
      AddStatusToCommandStatusQueue(&FailStatus);
    }
    else
    {
      // A frame with callback has entered protocol
      // All frames without callback should not increment the variable as there are no callback to decremented again.
      if (NULL != TxCallback.pCallback)
      {
        m_iFramesInProtocol++;
      }
    }

    // pull the handled or illegal frame from the queue
    xQueueReceive(m_pAppInterface->AppHandles.pZwTxQueue->Queue, (uint8_t*)pTransmitPackage, 0);
  }

  // Ensure we dont have items in queue, but no frames being processed in protocol.
  // If we do, the queue items will not be handled until another item is put on the queue,
  // And thats no good.
  ASSERT((0 != m_iFramesInProtocol) || (0 == uxQueueMessagesWaiting(m_pAppInterface->AppHandles.pZwTxQueue->Queue)));
}


/**
* Method used for callback from Protocol on Tx frame status.
*
* Puts the status on the queue to App and starts processing any pending
* entries on the App->ZW Tx queue.
*
* @param[in]     Context      Handle related to Tx frame delivered by App when
*                             requesting Tx. Returned to App along with Status.
* @param[in]     TxStatus     Status on Tx returned from protocol.
* @param[in]     pTxStatusReport  Extended Status on Tx returned from protocol.
*/
static void ZCB_SendDataCallback(ZW_Void_Function_t Context, uint8_t TxStatus, TX_STATUS_TYPE* pTxStatusReport)
{
  // A frame has exited protocol
  if (m_iFramesInProtocol) {
    m_iFramesInProtocol--;
  }

  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_TX,
    .Content.TxStatus.Handle = Context,
    .Content.TxStatus.bIsTxFrameLegal = true,
    .Content.TxStatus.TxStatus = TxStatus,
  };
  if (pTxStatusReport)
  {
    StatusPackage.Content.TxStatus.ExtendedTxStatus = *pTxStatusReport;
  }
  AddStatusToCommandStatusQueue(&StatusPackage);

  // Check to see if there is anything on Tx queue we need to handle.
  ProtocolInterfaceAppTxEventUpdate();
}


/**
* Method for putting Status on Command Status queue for APP.
*
* Puts the status on the queue to App and updates FramesInProtocol count.
*
* @param[in]     pStatusPackage   Pointer for item to put on command
*                                 status queue for app.
*/
static bool AddStatusToCommandStatusQueue(SZwaveCommandStatusPackage* pStatusPackage)
{
  EQueueNotifyingStatus QueueStatus = QueueNotifyingSendToBack(
                                                                &m_CommandStatusNotifyingQueue,
                                                                (uint8_t*)(pStatusPackage),
                                                                0
                                                               );
  if (EQUEUENOTIFYING_STATUS_SUCCESS != QueueStatus)
  {
    DPRINTF("ERROR: Failed to put Status on command %x status queue\r\n",pStatusPackage->eStatusType);
  }

  DPRINTF("Put command Status %x on Queue\r\n", pStatusPackage->eStatusType);

  return EQUEUENOTIFYING_STATUS_SUCCESS == QueueStatus;
}


/**
* Method used for callback from Protocol on Tx frame status, when App
* should not receive a status.
*
* Starts processing any pending entries on the App->ZW Tx queue.
*
* @param[in]     Context      Unused - Only here to comply with TransmitCallback
* @param[in]     TxStatus     Unused - Only here to comply with TransmitCallback
* @param[in]     pTxStatusReport  Unused - Only here to comply with TransmitCallback
*/
static void ZCB_ExplorerRequestClusion( __attribute__((unused)) ZW_Void_Function_t Context, __attribute__((unused)) uint8_t TxStatus,
                                        __attribute__((unused)) TX_STATUS_TYPE* pTxStatusReport)
{
  // A frame has exited protocol
  if (m_iFramesInProtocol) {
    m_iFramesInProtocol--;
  }
  m_nwl = false;
  // Check to see if there is anything on Tx queue we need to handle.
  ProtocolInterfaceAppTxEventUpdate();
}


#ifdef ZW_CONTROLLER

static void ZCB_NetworkManagementCallback(LEARN_INFO_T* pLearnInfo);

static void ZCB_RequestNodeNeighborUpdate(ZW_Void_Function_t Context, uint8_t TxStatus, __attribute__((unused)) TX_STATUS_TYPE* pTxStatusReport)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_TX,
    .Content.TxStatus.Handle = Context,
    .Content.TxStatus.bIsTxFrameLegal = true,
    .Content.TxStatus.TxStatus = TxStatus,
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

static void ZCB_RequestNodeTypeNeighborUpdate(ZW_Void_Function_t Context, uint8_t TxStatus, __attribute__((unused)) TX_STATUS_TYPE* pTxStatusReport)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_TX,
    .Content.TxStatus.Handle = Context,
    .Content.TxStatus.bIsTxFrameLegal = true,
    .Content.TxStatus.TxStatus = TxStatus,
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

static void ZCB_NetworkManagementCallback(LEARN_INFO_T* pLearnInfo)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_NETWORK_MANAGEMENT,
    .Content.NetworkManagementStatus.statusInfo[0] = pLearnInfo->bStatus,
    .Content.NetworkManagementStatus.statusInfo[1] = pLearnInfo->bSource >> 8, // nodeID MSB
    .Content.NetworkManagementStatus.statusInfo[2] = pLearnInfo->bSource,      // nodeID LSB
    .Content.NetworkManagementStatus.statusInfo[3] = pLearnInfo->bLen

  };
  memcpy(&StatusPackage.Content.NetworkManagementStatus.statusInfo[4], pLearnInfo->pCmd, pLearnInfo->bLen);
  AddStatusToCommandStatusQueue(&StatusPackage);
}

#ifdef ZW_CONTROLLER_BRIDGE
static void ZCB_SetSlaveLearnModeCallback(uint8_t bStatus, node_id_t orgID, node_id_t newID);
static void ZCB_SetSlaveLearnModeCallback(uint8_t bStatus, node_id_t orgID, node_id_t newID)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_SET_SLAVE_LEARN_MODE,
    .Content.NetworkManagementStatus.statusInfo[0] = bStatus,
    .Content.NetworkManagementStatus.statusInfo[1] = (uint8_t)(orgID >> 8),    // org nodeID MSB
    .Content.NetworkManagementStatus.statusInfo[2] = (uint8_t)(orgID & 0xFF),  // org nodeID LSB
    .Content.NetworkManagementStatus.statusInfo[3] = (uint8_t)(newID >> 8),    // new nodeID MSB
    .Content.NetworkManagementStatus.statusInfo[4] = (uint8_t)(newID & 0xFF),  // new nodeID LSB

  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}
#endif
#endif

static void handleAppCommand(
    SZwaveCommandPackage* CommandPackage,
    SZwaveCommandStatusPackage* StatusPackage,
    bool* bSendStatus,
    bool* bPullFromQueueAfterHandling)
{
  switch (CommandPackage->eCommandType)
  {
    case EZWAVECOMMANDTYPE_GET_BACKGROUND_RSSI:
    {
      memset(StatusPackage->Content.GetBackgroundRssiStatus.rssi, ZPAL_RADIO_RSSI_NOT_AVAILABLE, sizeof(SZWaveGetBackgroundRssiStatus));
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GET_BACKGROUND_RSSI;
      ZW_GetBackgroundRSSI((RSSI_LEVELS*)&StatusPackage->Content.GetBackgroundRssiStatus.rssi[0]);
      break;
    }

    case EZWAVECOMMANDTYPE_GENERATE_RANDOM:
    {
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GENERATE_RANDOM;
      uint8_t rndBytes = CommandPackage->uCommandParams.GenerateRandom.iLength;

      if (rndBytes > sizeof(StatusPackage->Content.GenerateRandomStatus.aRandomNumber)) {
        StatusPackage->Content.GenerateRandomStatus.iLength = 0;
        break;
      }

      StatusPackage->Content.GenerateRandomStatus.iLength = rndBytes;
      zpal_get_random_data(StatusPackage->Content.GenerateRandomStatus.aRandomNumber, rndBytes);
      break;
    }

#ifdef ZW_CONTROLLER
    case EZWAVECOMMANDTYPE_GET_PRIORITY_ROUTE:
    {
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GET_PRIORITY_ROUTE;
      // dummy values for now
      StatusPackage->Content.GetPriorityRouteStatus.bAnyRouteFound =
        ZW_GetPriorityRoute(CommandPackage->uCommandParams.GetPriorityRoute.nodeID,
                            StatusPackage->Content.GetPriorityRouteStatus.repeaters);
      break;
    }

    case EZWAVECOMMANDTYPE_NODE_INFO:
    {
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NODE_INFO;
      node_id_t NodeId = CommandPackage->uCommandParams.NodeInfo.NodeId;
      StatusPackage->Content.NodeInfoStatus.NodeId = NodeId;
      StatusPackage->Content.NodeInfoStatus.extNodeInfo.extInfo = 0;
      ZW_GetNodeProtocolInfo(NodeId, &StatusPackage->Content.NodeInfoStatus.extNodeInfo.NodeInfo);
      if (CtrlStorageLongRangeGet(NodeId))
      {
        StatusPackage->Content.NodeInfoStatus.extNodeInfo.extInfo |= 0x01;
      }
      break;
    }
#endif // #ifdef ZW_CONTROLLER

    case EZWAVECOMMANDTYPE_CLEAR_NETWORK_STATISTICS:
    {
      zpal_radio_clear_network_stats();
      *bSendStatus = false;  // Command is fire and forget, we expect it to happen instantly
      break;
    }

    case EZWAVECOMMANDTYPE_CLEAR_TX_TIMERS:
    {
      zpal_radio_clear_tx_timers();
      *bSendStatus = false;  // Command is fire and forget, we expect it to happen instantly
      break;
    }

    case EZWAVECOMMANDTYPE_SET_LEARN_MODE:
    {
      STATIC_ASSERT((ZW_SET_LEARN_MODE_DISABLE == ELEARNMODE_DISABLED), STATIC_ASSERT_FAILED_Interface_protocol_mismatch_learndis);
      STATIC_ASSERT((ZW_SET_LEARN_MODE_CLASSIC == ELEARNMODE_CLASSIC), STATIC_ASSERT_FAILED_Interface_protocol_mismatch_learnclas);
      STATIC_ASSERT((ZW_SET_LEARN_MODE_NWI == ELEARNMODE_NETWORK_WIDE_INCLUSION), STATIC_ASSERT_FAILED_Interface_protocol_mismatch_learnnwi);
      STATIC_ASSERT((ZW_SET_LEARN_MODE_NWE == ELEARNMODE_NETWORK_WIDE_EXCLUSION), STATIC_ASSERT_FAILED_Interface_protocol_mismatch_learnnwe);

      *bSendStatus = false;  // Command changes mode, protocol will perform status callbacks

      ELearnMode eLearnMode = CommandPackage->uCommandParams.SetLearnMode.eLearnMode;
      uint8_t useCallback =  CommandPackage->uCommandParams.SetLearnMode.useCB;
      ZW_SetLearnMode(eLearnMode, useCallback ? ZCB_LearnModeStatus: NULL);
      DPRINTF("Learn mode %d\r\n", eLearnMode);
      break;
    }

    case EZWAVECOMMANDTYPE_SET_DEFAULT:
    {
      ZW_SetDefault();
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_SET_DEFAULT;
      *bPullFromQueueAfterHandling = false;
      break;
    }

    case EZWAVECOMMANDTYPE_SEND_DATA_ABORT:
    {
      ZW_SendDataAbort();
      *bSendStatus = false;  // Protocol will perform status callbacks
      break;
    }

    case EZWAVECOMMANDTYPE_SET_RF_RECEIVE_MODE:
    {
      StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_SET_RF_RECEIVE_MODE;
      StatusPackage->Content.SetRFReceiveModeStatus.result = ZW_SetRFReceiveMode(CommandPackage->uCommandParams.SetRfReceiveMode.mode);
      break;
    }

    case EZWAVECOMMANDTYPE_SOFT_RESET:
    {
      *bSendStatus = false;
#ifdef ZW_CONTROLLER
      //Store NodeRouteCaches to NVM before reset
      StoreNodeRouteCacheBuffer();
#endif
      DPRINTF("Soft reset command received\n");
      zpal_reboot_with_info(MFG_ID_ZWAVE_ALLIANCE, ZPAL_RESET_REQUESTED_BY_SAPI);
      break;
    }

#ifdef ZW_CONTROLLER

  case EZWAVECOMMANDTYPE_REQUESTNODENEIGHBORUPDATE:
  {
    STransmitCallback TxCallback;
    SCommandRequestNodeNeighborUpdate* pRequestNodeNeighborUpdate = &CommandPackage->uCommandParams.RequestNodeNeighborUpdate;
    ZW_TransmitCallbackBind(&TxCallback, ZCB_RequestNodeNeighborUpdate, pRequestNodeNeighborUpdate->Handle);
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_REQUESTNODENEIGHBORUPDATE;
    StatusPackage->Content.RequestNodeNeigborUpdateStatus.result = ZW_RequestNodeNeighborUpdate(
      pRequestNodeNeighborUpdate->NodeId,
      &TxCallback
      );
    break;
  }

  case EZWAVECOMMANDTYPE_GET_CONTROLLER_CAPABILITIES:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GET_CONTROLLER_CAPABILITIES;
    StatusPackage->Content.GetControllerCapabilitiesStatus.result = ZW_GetControllerCapabilities();
    break;
  }

  case EZWAVECOMMANDTYPE_IS_PRIMARY_CTRL:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_IS_PRIMARY_CTRL;
    StatusPackage->Content.IsPrimaryCtrlStatus.result = ZW_IsPrimaryCtrl();
    break;
  }

  case EZWAVECOMMANDTYPE_ADD_NODE_TO_NETWORK:
  {
    *bSendStatus = false;
    uint8_t useCallback = CommandPackage->uCommandParams.NetworkManagement.pHandle != NULL;
    DPRINTF("AddNodeToNetwork 0x%x, 0x%x\r\n",
        CommandPackage->uCommandParams.NetworkManagement.mode, useCallback);

    ZW_AddNodeToNetwork(CommandPackage->uCommandParams.NetworkManagement.mode,
                        useCallback ? ZCB_NetworkManagementCallback: NULL);
    break;
  }

  case EZWAVECOMMANDTYPE_REMOVE_NODEID_FROM_NETWORK:
  case EZWAVECOMMANDTYPE_REMOVE_NODE_FROM_NETWORK:
  {
    *bSendStatus = false;
    uint8_t useCallback = CommandPackage->uCommandParams.NetworkManagement.pHandle != NULL;
    DPRINTF("RemoveNodeIDFromNetwork 0x%x, 0x%x\r\n",
        CommandPackage->uCommandParams.NetworkManagement.mode, useCallback);
    ZW_RemoveNodeIDFromNetwork(CommandPackage->uCommandParams.NetworkManagement.mode,
                               CommandPackage->uCommandParams.NetworkManagement.nodeID,
                               useCallback ? ZCB_NetworkManagementCallback: NULL);
    break;
  }
  case EZWAVECOMMANDTYPE_REMOVE_FAILED_NODE_ID:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_REMOVE_FAILED_NODE_ID;
    StatusPackage->Content.FailedNodeIDStatus.result =  ZW_RemoveFailedNode(
    CommandPackage->uCommandParams.FailedNodeIDCmd.nodeID, RemoveFailedNodeIDCB);
    break;
  }

  case EZWAVECOMMANDTYPE_CONTROLLER_CHANGE:
  {
    *bSendStatus = false;
    uint8_t useCallback = CommandPackage->uCommandParams.NetworkManagement.pHandle != NULL;
    DPRINTF("ControllerChange 0x%x, 0x%x\r\n",
        CommandPackage->uCommandParams.NetworkManagement.mode, useCallback);
    ZW_ControllerChange(CommandPackage->uCommandParams.NetworkManagement.mode,
                        useCallback ? ZCB_NetworkManagementCallback: NULL);
    break;
  }

  #ifdef ZW_CONTROLLER_BRIDGE
  case EZWAVECOMMANDTYPE_IS_VIRTUAL_NODE:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_IS_VIRTUAL_NODE;
    StatusPackage->Content.IsVirtualNodeStatus.result = CtrlStorageGetBridgeNodeFlag(CommandPackage->uCommandParams.IsVirtualNode.value);
    break;
  }
  case EZWAVECOMMANDTYPE_GET_VIRTUAL_NODES:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GET_VIRTUAL_NODES;
    CtrlStorageReadBridgeNodePool(StatusPackage->Content.GetVirtualNodesStatus.vNodesMask);
    break;
  }

  case EZWAVECOMMANDTYPE_SET_SLAVE_LEARN_MODE:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_SET_SLAVE_LEARN_MODE_RESULT;
    StatusPackage->Content.SetSlaveLearnModeStatus.result =
        ZW_SetSlaveLearnMode(CommandPackage->uCommandParams.NetworkManagement.nodeID,
                             CommandPackage->uCommandParams.NetworkManagement.mode,
                             ZCB_SetSlaveLearnModeCallback);
    break;
  }
#endif //ZW_CONTROLLER_BRIDGE

  case EZWAVECOMMANDTYPE_IS_FAILED_NODE_ID:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_IS_FAILED_NODE_ID;
    StatusPackage->Content.FailedNodeIDStatus.result = ZW_isFailedNode(CommandPackage->uCommandParams.IsFailedNodeID.nodeID);
    break;
  }

  case EZWAVECOMMANDTYPE_SET_PRIORITY_ROUTE:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_SET_PRIORITY_ROUTE;
    uint8_t *result = &StatusPackage->Content.SetPriorityRouteStatus.bRouteUpdated;
    if (CommandPackage->uCommandParams.SetPriorityRoute.clearGolden)
    {
      *result = ZW_SetPriorityRoute(CommandPackage->uCommandParams.SetPriorityRoute.nodeID,
                                    NULL);
    }
    else
    {
      *result = ZW_SetPriorityRoute(CommandPackage->uCommandParams.SetPriorityRoute.nodeID,
                                    CommandPackage->uCommandParams.SetPriorityRoute.repeaters);
    }
    break;
  }

  case EZWAVECOMMANDTYPE_LOCK_ROUTE_RESPONSE:
  {
    *bSendStatus = false;
    ZW_LockRoute(CommandPackage->uCommandParams.LockRouteResponse.value);
    break;
  }
  case EZWAVECOMMANDTYPE_REPLACE_FAILED_NODE_ID:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_REPLACE_FAILED_NODE_ID;
    StatusPackage->Content.FailedNodeIDStatus.result = ZW_ReplaceFailedNode(
                                                        CommandPackage->uCommandParams.FailedNodeIDCmd.nodeID,
                                                        /* Use NormalPower for including the replacement */
                                                         true, ReplaceFailedNodeIDCB);
    break;
  }

  case EZWAVECOMMANDTYPE_STORE_HOMEID:
  {
    *bSendStatus = false;
    break;
  }

  case EZWAVECOMMANDTYPE_GET_ROUTING_TABLE_LINE:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_GET_ROUTING_TABLE_LINE;
    ZW_GetRoutingInfo(CommandPackage->uCommandParams.GetRoutingInfo.nodeID,
                      StatusPackage->Content.GetRoutingInfoStatus.RoutingInfo,
                      CommandPackage->uCommandParams.GetRoutingInfo.options);
    break;
  }

  case EZWAVECOMMANDTYPE_ZW_UPDATE_CTRL_NODE_INFORMATION:
  {
    *bSendStatus = false;
    ZW_UpdateCtrlNodeInformation( CommandPackage->uCommandParams.UpdateCtrlNodeInformation.value);
    break;
  }

  case EZWAVECOMMANDTYPE_ADD_NODE_DSK_TO_NETWORK:
  {
    *bSendStatus = false;
    uint8_t useCallback = CommandPackage->uCommandParams.NetworkManagementDSK.pHandle != NULL;
    DPRINTF("AddNodeToNetwork 0x%x, 0x%x\r\n",
            CommandPackage->uCommandParams.NetworkManagementDSK.mode, useCallback);
    ZW_AddNodeDskToNetwork(CommandPackage->uCommandParams.NetworkManagementDSK.mode,
                           &CommandPackage->uCommandParams.NetworkManagementDSK.dsk[0],
                           useCallback ? ZCB_NetworkManagementCallback: NULL);
    break;
  }

  case EZWAVECOMMANDTYPE_NVM_BACKUP_OPEN:
  {
    /*Store dynamic routes to nvm*/
    StoreNodeRouteCacheBuffer();
    /*Shut down RF disable power management*/
    zpal_radio_power_down();
    ZwTimerStopAll();
    TxQueueInit();
    const zpal_status_t status = zpal_nvm_backup_open();
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NVM_BACKUP_RESTORE;
    StatusPackage->Content.NvmBackupRestoreStatus.status = (ZPAL_STATUS_OK == status) ? true : false;
    break;
  }

  case EZWAVECOMMANDTYPE_NVM_BACKUP_CLOSE:
  {
    *bSendStatus = false;
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NVM_BACKUP_RESTORE;
    zpal_nvm_backup_close();
    break;
  }

  case EZWAVECOMMANDTYPE_NVM_BACKUP_READ:
  {

    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NVM_BACKUP_RESTORE;
    StatusPackage->Content.NvmBackupRestoreStatus.status = true;
    uint32_t offset = CommandPackage->uCommandParams.NvmBackupRestore.offset;
    uint32_t length = CommandPackage->uCommandParams.NvmBackupRestore.length;
    uint8_t* nvmData = CommandPackage->uCommandParams.NvmBackupRestore.nvmData;
    DPRINTF("NVM_READ 0x%08x, 0x%08x, 0x%08x\r\n",offset, length, (uint32_t)nvmData);
    zpal_nvm_backup_read(offset, nvmData, length);
    break;
  }

  case EZWAVECOMMANDTYPE_NVM_BACKUP_WRITE:
  {
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NVM_BACKUP_RESTORE;
    StatusPackage->Content.NvmBackupRestoreStatus.status = true;
    uint32_t offset = CommandPackage->uCommandParams.NvmBackupRestore.offset;
    uint32_t length = CommandPackage->uCommandParams.NvmBackupRestore.length;
    uint8_t* nvmData = CommandPackage->uCommandParams.NvmBackupRestore.nvmData;
    DPRINTF("NVM_Write_par 0x%08x, 0x%08x, 0x%08x\r\n", offset, length, (uint32_t)nvmData);
    const zpal_status_t status = zpal_nvm_backup_write(offset, nvmData, length);
    StatusPackage->Content.NvmBackupRestoreStatus.status = (ZPAL_STATUS_OK == status) ? true : false;
    break;
  }

  case EZWAVECOMMANDTYPE_SET_ROUTING_MAX:
  {
    *bSendStatus = false;
    ZW_SetRoutingMAX(CommandPackage->uCommandParams.SetRoutingMax.value);
    break;
  }
  case EZWAVECOMMANDTYPE_ZW_GET_INCLUDED_LR_NODES:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_GET_INCLUDED_LR_NODES;
    GetIncludedLrNodes(StatusPackage->Content.GetIncludedNodesLR.node_id_list);
  break;

  case EZWAVECOMMANDTYPE_ZW_GET_INCLUDED_NODES:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_GET_INCLUDED_NODES;
    GetIncludedNodes(StatusPackage->Content.GetIncludedNodes.node_id_list);
  break;

  case EZWAVECOMMANDTYPE_ZW_INITIATE_SHUTDOWN:
    *bPullFromQueueAfterHandling = false;
    StatusPackage->Content.InitiateShutdownStatus.result = true;
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_INITIATE_SHUTDOWN;
    ZW_initiate_shutdown(CommandPackage->uCommandParams.InitiateShutdown.Handle);
    //  Reset all queues To make sure no pending elements are in the queues, in this way we enter into deep sleep as fast as possible.
    //  Keep in mind that this feature graceful shutdown is only used when we have power shortage situation, so we don't need to handle
    // any other command.
    ProtocolInterfaceReset();
  break;

  case EZWAVECOMMANDTYPE_ZW_GET_LR_CHANNEL:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_GET_LR_CHANNEL;
    StatusPackage->Content.GetLRChannel.result = ProtocolLongRangeChannelGet();
  break;

  case EZWAVECOMMANDTYPE_ZW_SET_LR_CHANNEL:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_LR_CHANNEL;
    StatusPackage->Content.GetLRChannel.result =
    ProtocolLongRangeChannelSet(CommandPackage->uCommandParams.SetLRChannel.value);
  break;

  case EZWAVECOMMANDTYPE_ZW_SET_LR_VIRTUAL_IDS:
    *bSendStatus = false; // Command is fire and forget, we expect it to happen instantly
    ZW_LR_EnableVirtualIDs(CommandPackage->uCommandParams.SetLRVirtualNodeIDs.value);
  break;

  case EZWAVECOMMANDTYPE_ZW_GET_PTI_CONFIG:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_GET_PTI_CONFIG;
    StatusPackage->Content.GetPTIconfig.result = zpal_radio_is_debug_enabled();
  break;

  case EZWAVECOMMANDTYPE_REQUESTNODETYPE_NEIGHBORUPDATE:
  {
    STransmitCallback TxCallback;
    ZW_TransmitCallbackBind(&TxCallback, ZCB_RequestNodeTypeNeighborUpdate,
                            CommandPackage->uCommandParams.RequestNodeTypeNeighborUpdate.Handle);
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_REQUESTNODETYPE_NEIGHBORUPDATE;
    StatusPackage->Content.RequestNodeTypeNeigborUpdateStatus.result = ZW_RequestNodeTypeNeighborUpdate(
      CommandPackage->uCommandParams.RequestNodeTypeNeighborUpdate.NodeId,
      CommandPackage->uCommandParams.RequestNodeTypeNeighborUpdate.NodeType,
      &TxCallback
      );
  }
  break;

#endif //ZW_CONTROLLER

#ifdef ZW_PROMISCUOUS_MODE
  case EZWAVECOMMANDTYPE_SET_PROMISCUOUS_MODE:
  {
    *bSendStatus = false; // Command is fire and forget, we expect it to happen instantly
    StatusPackage->eStatusType = EZWAVECOMMANDTYPE_SET_PROMISCUOUS_MODE;
    ZW_SET_PROMISCUOUS_MODE(CommandPackage->uCommandParams.SetPromiscuousMode.Enable);
    break;
  }
#endif

  case EZWAVECOMMANDTYPE_PM_SET_POWERDOWN_CALLBACK:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_PM_SET_POWERDOWN_CALLBACK;
    StatusPackage->Content.SetPowerDownCallbackStatus.result = ZW_SetAppPowerDownCallback(CommandPackage->uCommandParams.PMSetPowerDownCallback.callback);
    break;

  case EZWAVECOMMANDTYPE_ZW_SET_LBT_THRESHOLD:
    *bSendStatus = false;
    zpal_radio_set_lbt_level(CommandPackage->uCommandParams.SetLBTThreshold.channel, CommandPackage->uCommandParams.SetLBTThreshold.level);
    break;

  case EZWAVECOMMANDTYPE_NETWORK_LEARN_MODE_START:
      // Protocol interface module does not translate input param, so we check if they match compiletime
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_NETWORK_LEARN_MODE_START;
    E_NETWORK_LEARN_MODE_ACTION eLearnMode = CommandPackage->uCommandParams.SetSmartStartLearnMode.eLearnMode;
    StatusPackage->Content.NetworkManagementStatus.statusInfo[0] = ZW_NetworkLearnModeStart(eLearnMode);
    DPRINTF("Learn mode %d\r\n", eLearnMode);
    break;

  case EZWAVECOMMANDTYPE_ZW_SET_MAX_INCL_REQ_INTERVALS:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_MAX_INCL_REQ_INTERVALS;
    uint32_t setInterval = CommandPackage->uCommandParams.SetMaxInclReqInterval.inclusionRequestInterval;
    uint32_t returnInterval = ZW_NetworkManagementSetMaxInclusionRequestIntervals(setInterval);
    StatusPackage->Content.NetworkManagementStatus.statusInfo[0] = (returnInterval == setInterval) ? true : false;
    break;

#ifndef ZW_CONTROLLER
  case EZWAVECOMMANDTYPE_SET_SECURITY_KEYS:
    *bSendStatus = false;
    ZW_s2_inclusion_init(CommandPackage->uCommandParams.SetSecurityKeys.keys);
    break;
#endif

  case EZWAVECOMMANDTYPE_BOOTLOADER_REBOOT:
    *bSendStatus = false;
#ifdef ZW_CONTROLLER
    StoreNodeRouteCacheBuffer();
#endif
#ifdef ZW_SECURITY_PROTOCOL
    sec2_PowerDownHandler();
#endif
    zpal_bootloader_reboot_and_install();
    break;

  case EZWAVECOMMANDTYPE_ZW_SET_TX_ATTENUATION:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_SET_TX_ATTENUATION;
    bool retTxAtt = zpal_radio_attenuate(CommandPackage->uCommandParams.SetTxAttenuation.value);
    StatusPackage->Content.SetTxAttenuation.result = retTxAtt;
  break;

  case EZWAVECOMMANDTYPE_ZW_GET_TX_POWER_MAX_SUPPORTED:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_ZW_GET_TX_POWER_MAX_SUPPORTED;
    StatusPackage->Content.GetTxPowerMaximumSupported.tx_power_max_supported = zpal_radio_get_maximum_tx_power();
    break;

  default:
    StatusPackage->eStatusType = EZWAVECOMMANDSTATUS_INVALID_COMMAND;
    StatusPackage->Content.InvalidCommandStatus.InvalidCommand = CommandPackage->eCommandType;
    DPRINTF("Error: Unknown ZwCommandQueue Command type %d\r\n", CommandPackage->eCommandType);
    break;
  }
}

void ProtocolInterfaceAppCommandHandler(void)
{
  SZwaveCommandPackage CommandPackage = { 0 };
  SZwaveCommandStatusPackage StatusPackage = { 0 };

  // Peek command at first, so it is still there if we are unable to handle it right now
  while (xQueuePeek(m_pAppInterface->AppHandles.pZwCommandQueue->Queue, (uint8_t*)&CommandPackage, 0) == pdTRUE)
  {
    DPRINTF("CommandQueue Handling %d\r\n", CommandPackage.eCommandType);

    bool bSendStatus = true;
    bool bPullFromQueueAfterHandling = true;

    // Contains a huge switch-statement that handles all commands coming from the app.
    handleAppCommand(&CommandPackage, &StatusPackage, &bSendStatus, &bPullFromQueueAfterHandling);

    // pull the handled or unknown command from the queue if required
    if (bPullFromQueueAfterHandling == true)
    {
      xQueueReceive(m_pAppInterface->AppHandles.pZwCommandQueue->Queue, (uint8_t*)&CommandPackage, 0);
    }

    // Put command status on queue if required
    if (bSendStatus)
    {
      AddStatusToCommandStatusQueue(&StatusPackage);
    }
  }
}


#ifdef ZW_CONTROLLER
/**
* Method used for set Learn node state status callback from Protocol.
*
* Puts the learn mode status on the queue to App.
*
* @param[in]     pLearnStatus   Pointer to struct containing status
*/
static void ZCB_LearnModeStatus(LEARN_INFO_T* pLearnStatus)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS,
    .Content.LearnModeStatus.Status = pLearnStatus->bStatus
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

void ProtocolInterfacePassToAppLearnStatus(LEARN_INFO_T* pLearnStatus)
{
  ZCB_LearnModeStatus(pLearnStatus);
}
#endif // #ifdef ZW_CONTROLLER
#ifdef ZW_SLAVE
/**
* Method used for Learn mode status callback from Protocol.
*
* Puts the learn mode status on the queue to App.
*
* @param[in]     Status     Learn mode status
* @param[in]     NodeId     NodeId received on inclusion (only valid in combination with certain status values)
*/
static void ZCB_LearnModeStatus(ELearnStatus Status, __attribute__((unused)) node_id_t NodeId)
{
  SZwaveCommandStatusPackage StatusPackage = {
    .eStatusType = EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS,
    .Content.LearnModeStatus.Status = Status
  };
  AddStatusToCommandStatusQueue(&StatusPackage);
}

void ProtocolInterfacePassToAppLearnStatus(ELearnStatus Status)
{
  ZCB_LearnModeStatus(Status, 0);
}

#endif // #ifdef ZW_SLAVE


void ProtocolInterfaceSucNodeIdSet(uint32_t SucNodeId)
{
  m_NetworkInfo.SucNodeId = SucNodeId;
}


void ProtocolInterfaceRadioPowerLevelSet(int32_t iRadioPowerLevel)
{
  m_RadioStatus.iRadioPowerLevel = iRadioPowerLevel;
}

void ProtocolInterfaceSecurityKeysSet(uint32_t SecurityKeys)
{
  m_NetworkInfo.SecurityKeys = SecurityKeys;

  InclusionStateUpdate();
  MaxPayloadSizeUpdate();
}


void ProtocolInterfaceHomeIdSet(uint32_t HomeId, uint32_t NodeId)
{
  m_NetworkInfo.HomeId = HomeId;
  m_NetworkInfo.NodeId = NodeId;

  InclusionStateUpdate();
}


static void InclusionStateUpdate(void)
{
  if (0 == m_NetworkInfo.NodeId)
  {
    m_NetworkInfo.eInclusionState = EINCLUSIONSTATE_EXCLUDED;
    return;
  }

  if (SECURITY_KEY_NONE_MASK == m_NetworkInfo.SecurityKeys)
  {
    m_NetworkInfo.eInclusionState = EINCLUSIONSTATE_UNSECURE_INCLUDED;
    return;
  }

  m_NetworkInfo.eInclusionState = EINCLUSIONSTATE_SECURE_INCLUDED;
}

static void MaxPayloadSizeUpdate(void)
{
  m_NetworkInfo.MaxPayloadSize = ZW_GetMaxPayloadSize(m_NetworkInfo.SecurityKeys);

#ifdef ZW_CONTROLLER
  m_LongRangeInfo.MaxLongRangePayloadSize = ZW_GetMaxLRPayloadSize();
#endif
}

uint32_t
ProtocolInterfaceFramesToAppQueueFull(void)
{
  return (uxQueueSpacesAvailable(m_pAppInterface->AppHandles.ZwRxQueue) == 0);
}
