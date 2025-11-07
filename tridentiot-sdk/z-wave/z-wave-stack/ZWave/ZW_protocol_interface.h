// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol_interface.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief ProtocolInterface Module.
 * Module is the ZWave protocol side implementation of the interface
 * for applications to communicate with ZWave.
 */
#ifndef _ZW_PROTOCOL_INTERFACE_H_
#define _ZW_PROTOCOL_INTERFACE_H_
#include "FreeRTOS.h"
#include "queue.h"
#include "SyncEvent.h"
#include "ZW_transport_api.h"
#include "ZW_classcmd.h"
#include "ZW_application_transport_interface.h" // For zpal_radio_network_stats_t
#ifdef ZW_CONTROLLER
#include "ZW_controller_api.h"
#endif  // ZW_CONTROLLER

/**< Struct passed to protocol task for communicating with Application */
typedef struct SApplicationInterface
{
  TaskHandle_t                ApplicationTaskHandle;
  /**< FreeRTOS Task notification bit Set by Protocol when a message has been put on the ZW Rx queue. */
  uint8_t                     iZwRxQueueTaskNotificationBitNumber;

  /**< FreeRTOS Task notification bit Set by Protocol when a message has been put on the ZW command status queue. */
  uint8_t                     iZwCommandStatusQueueTaskNotificationBitNumber;

  /**< Configuration info from application to Protocol */
  const SProtocolConfig_t *   pProtocolConfig;

  /**< Node wakeup reason */
  zpal_reset_reason_t   wakeupReason;

  SApplicationHandles AppHandles;

  SCommandClassSet_t *CCSet;
} SApplicationInterface;


/**
* Init for ProtocolInterface module. Must be called prior to calling any
* other module methods.
* @param[in]    pAppInterface                 Pointer to struct with information about application,
*                                             needed for communication with application.
* @param[in]    AppTxHandlerEventBitNumber    Bit number for notifying Protocol
*                                             Task that there is items in Tx Queue.
* @param[in]    AppCommandHandlerEventBitNumber Bit number for notifying Protocol Task
*                                               that there is items in command queue
*/
void ProtocolInterfaceInit(
                            SApplicationInterface* pAppInterface,
                            uint8_t AppTxHandlerEventBitNumber,
                            uint8_t AppCommandHandlerEventBitNumber
                          );


/**
* Reset for ProtocolInterface module.
*
* Resets interface Queues and internal variables of Protocol Interface.
* Status for app that is controlled via "setters" (e.g. ProtocolInterfaceSucNodeIdSet()),
* are not touched by reset.
*/
void ProtocolInterfaceReset(void);


/**
* Method for handling incoming Tx requests from application to ZW.
* Method should be called when an item is put on the App->Zw TX queue.
*
* The Protocol interface module will recheck App->Zw Tx queue for
* pending entries when the Protocol has finished processing the previous
* entry.
*/
void ProtocolInterfaceAppTxHandler(void);


/**
* Method for handling incoming commands from application to ZW.
* Method should be called when an item is put on the App->Zw Command queue.
*
* The Protocol interface module will recheck App->Zw command queue for
* pending entries when the Protocol has finished processing the previous
* entry.
*/
void ProtocolInterfaceAppCommandHandler(void);


#ifdef ZW_CONTROLLER_BRIDGE
/**
* Method passes received multicast frame from protocol to Application.
*
* @param[in]     NodeMaskOffset   Destination nodemask offset. Defines which node is
                                  reprsented bt first bit in NodeMask.
                                  NodeId of first bit in node mask = (NodeMaskOffset * 32) + 1
* @param[in]     NodeMaskLength   Length of nodemask (bytes) pointe to by pNodeMask
* @param[in]     pNodeMask        Pointer to varaible length frame destination nodemask
* @param[in]     pCommand         Pointer to received frame payload
* @param[in]     iCommandLength   Length of frame (bytes) pointed to by pCommand
* @param[in]     pRxOptions       Pointer to frame Rx Options
*
* @retval        true             Frame successfully put on queue to application
* @retval        false            Frame NOT put on queue. Queue is full.
*/
bool ProtocolInterfacePassToAppMultiFrame(
                                          uint8_t NodeMaskOffset,
                                          uint8_t NodeMaskLength,
                                          const uint8_t* pNodeMask,
                                          const ZW_APPLICATION_TX_BUFFER *pCommand,
                                          uint8_t iCommandLength,
                                          const RECEIVE_OPTIONS_TYPE *rxOpt
                                        );

#else // ZW_CONTROLLER_BRIDGE

/**
* Method passes received unicast frame from protocol to Application.
*
* @param[in]     pCommand         Pointer to received frame payload
* @param[in]     iCommandLength   Length of frame (bytes) pointed to by pCommand
* @param[in]     pRxOptions       Pointer to frame Rx Options
*
* @retval        true             Frame successfully put on queue to application
* @retval        false            Frame NOT put on queue. Queue is full.
*/
bool ProtocolInterfacePassToAppSingleFrame(
                                            uint8_t iCommandLength,
                                            const ZW_APPLICATION_TX_BUFFER *pCommand,
                                            const RECEIVE_OPTIONS_TYPE *pRxOptions
                                          );

#endif  // ZW_CONTROLLER_BRIDGE
/**
* Method passes received node update frame from protocol to Application.
*
* @param[in]     Status           Status of learn mode
* @param[in]     NodeId           Node ID of the node that sendt node info
* @param[in]     pCommand         Pointer to application node information
* @param[in]     iLength          Length (bytes) of info pointed to by pCommand
*
* @retval        true             Frame successfully put on queue to application
* @retval        false            Frame NOT put on queue. Queue is full.
*/
bool ProtocolInterfacePassToAppNodeUpdate(
                                          uint8_t Status,
                                          node_id_t NodeId,
                                          const uint8_t *pCommand,
                                          uint8_t iLength
                                          );

/**
* Method passes a security event from protocol to Application.
*
* @param[in]     pSecurityEvent   Pointer for security event
*
* @retval        true             Security event successfully put on queue to application
* @retval        false            Security event NOT put on queue. Queue is full.
*/
bool ProtocolInterfacePassToAppSecurityEvent(const s_application_security_event_data_t *pSecurityEvent);

/**
 * Informs the Application to stay awake.
 *
 * Used when protocol received and handled the frame after which
 * the Application should not go to sleep immediately.
 *
 * @retval true  Frame successfully put on queue to application
 * @retval false Frame NOT put on queue. Queue is full.
 */
bool ProtocolInterfacePassToAppStayAwake();

#ifdef ZW_SLAVE
/**
* Method passes learn mode status from protocol to Application.
*
* @param[in]     Status           Status of learn mode
*/
void ProtocolInterfacePassToAppLearnStatus(ELearnStatus Status);
#endif

#ifdef ZW_CONTROLLER
/**
* Method passes learn mode status from protocol to Application.
*
* @param[in]     pLearnStatus     Pointer to LEARN_INFO_T
*/
void ProtocolInterfacePassToAppLearnStatus(LEARN_INFO_T* pLearnStatus);
#endif

/**
* Method must be called when SUC NODE ID changes,
* so that app info can be updated.
*
* Updates application copy of SucNodeId
*
* @param[in]     SucNodeId    New SUC node ID
*/
void ProtocolInterfaceSucNodeIdSet(uint32_t SucNodeId);


/**
* Method must be called when Radio Power level changes,
* so that app info can be updated.
*
* Updates application copy of Radio power level
*
* @param[in]     iRadioPowerLevel   power level in db
*/
void ProtocolInterfaceRadioPowerLevelSet(int32_t iRadioPowerLevel);


/**
* Method must be called when possessed security keys changes,
* so that app info can be updated.
*
* Updates application copy of SecurityKeys and InclusionState.
*
* @param[in]     SecurityKeys   Security keys (a bit for each key).
*/
void ProtocolInterfaceSecurityKeysSet(uint32_t SecurityKeys);


/**
* Method must be called when Home ID or Node ID changes,
* so that app info can be updated.
*
* Updates application copy of Home ID, Node ID and Inclusion State.
*
* @param[in]     HomeId   Node Home ID
* @param[in]     NodeId   Node Id
*/
void ProtocolInterfaceHomeIdSet(uint32_t HomeId, uint32_t NodeId);

/**
* Return the status of the queue used for sendind the received application frames from
* protocol to application task
*
* @retval        true             The received application frames queue is full.
* @retval        false            The received application frames queue is not full.
*/
uint32_t ProtocolInterfaceFramesToAppQueueFull(void);


#endif /* _ZW_PROTOCOL_INTERFACE_H_ */
