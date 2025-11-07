// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_replication.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Controller replication functions.
 */
#ifndef _REPLICATION_H_
#define _REPLICATION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_transport_transmit_cb.h>
#include <ZW_controller_api.h>
#include <SwTimer.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
/* States for the replication state machine */
#define NEW_CTRL_STOP           0x00

#define NEW_CTRL_SEND           0x01
#define NEW_CTRL_SEND_NODES     0x02
#define NEW_CTRL_SEND_RANGE     0x03
#define NEW_CTRL_DELETE         0x04

#define NEW_CTRL_RECEIVE        0x05
#define NEW_CTRL_RECEIVE_INFO   0x06
#define NEW_CTRL_RECEIVE_NODES  0x07
#define NEW_CTRL_RECEIVE_RANGE  0x08

#define NEW_CTRL_CREATED        0x09

#define NEW_CTRL_STATUS_MASK      0x0f
#define RECEIVER_CTRL_BIT         0x10
#define WAIT_FOR_COMPLETE_BIT     0x20
#define SEND_TRANSFER_END_ON_STOP 0x40
#define REPLICATION_SEND_ONGOING       0x80 /*Indicates a replication send is underway*/

/*Macros that should be used for access to replicationStatus*/
#define SET_RECEIVER_BIT (replicationStatus|=RECEIVER_CTRL_BIT)
#define CLEAR_RECEIVER_BIT (replicationStatus&=(~RECEIVER_CTRL_BIT))
#define NEW_CTRL_RECEIVER_BIT (replicationStatus&(RECEIVER_CTRL_BIT))

#define SET_WAIT_FOR_COMPLETE_BIT (replicationStatus|=WAIT_FOR_COMPLETE_BIT)
#define CLEAR_WAIT_FOR_COMPLETE_BIT (replicationStatus&=(~WAIT_FOR_COMPLETE_BIT))
#define IS_WAIT_FOR_COMPLETE_BIT (replicationStatus&(WAIT_FOR_COMPLETE_BIT))

#define SET_SEND_END_BIT   (replicationStatus|=SEND_TRANSFER_END_ON_STOP)
#define CLEAR_SEND_END_BIT   (replicationStatus&=(~SEND_TRANSFER_END_ON_STOP))
#define DO_SEND_TRANSFER_END (replicationStatus&(SEND_TRANSFER_END_ON_STOP))

#define GET_NEW_CTRL_STATE  (replicationStatus&(NEW_CTRL_STATUS_MASK))
#define CHANGE_NEW_CTRL_STATE(value) (replicationStatus = (replicationStatus&(~NEW_CTRL_STATUS_MASK))|value)
#define INIT_NEW_CTRL_STATE(value)  replicationStatus=value

#define SET_REPLICATION_SEND_ONGOING (replicationStatus|=REPLICATION_SEND_ONGOING)
#define CLEAR_REPLICATION_SEND_ONGOING (replicationStatus&=(~REPLICATION_SEND_ONGOING))
#define IS_REPLICATION_SEND_ONGOING (replicationStatus&(REPLICATION_SEND_ONGOING))



/* Node ID of the other controller */
extern node_id_t otherController;

/* Current replication state */
extern uint8_t replicationStatus;

/* Number is increased for each package sent/received and should match the number received
  from the slave remote on CMD_COMPLETED*/
extern uint8_t sequenceNumber;


extern bool newPrimaryReplication;
#if defined(ZW_CONTROLLER_STATIC)
extern bool doChangeToSecondary;
#endif

/* Flag to indicate that we shall send ZWAVE_CMD_TRANSFER_PRESENTATION with high power. */
extern bool transferPresentationHighPower;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/*===================   ReplicationReceiveComplete   ========================
**
**    Send command complete frame to primary controller. Indicates that
**    the command received was executed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void
ZW_ReplicationReceiveComplete(void);

extern void
ZCB_WaitforRoutingAnalysis(SSwTimer* pTimer);

/*============================   ReplicationSend   ======================
**    Function description
**      Sends the payload to the receiver, which should respond with
**      command completed, with matching sequenceNumber.
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern uint8_t                              /*RET  false if transmitter busy      */
ReplicationSend(
  uint8_t      destNodeID,                  /*IN  Destination node ID. Only single cast allowed*/
  uint8_t     *pData,                       /*IN  Data buffer pointer           */
  uint8_t      dataLength,                  /*IN  Data buffer length            */
  TxOptions_t  txOptions,                   /*IN  Transmit option flags         */
  const STransmitCallback* pCompletedFunc); /*IN  Transmit completed call back function  */


/*======================   ReplicationSendDoneCallback ======================
**    Function description
**      Calls the application callback function with status
**      when a replication send have been completed
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationSendDoneCallback(
  uint8_t bStatus);                    /*IN TRANSMIT_COMPLETE_OK
                                    or TRANSMIT_COMPLETE_FAIL*/


/*===========================   ReplicationStart   ===========================
**
**    Set replication state so its ready to receive TRANSFER_INFO frames
**    and note the source ID of the controller including this controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationStart(
  uint8_t rxStatus,      /* IN  Frame header info */
  uint8_t bSource);


/*========================   PresentationReceived   ========================
**
**    Transfer presentation frame was received, set controller in learn
**    mode and send a node info frame.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
PresentationReceived( /* RET Nothing */
  uint8_t rxStatus,           /* IN  Frame header info */
  uint8_t bSource);           /* IN  Source id on controller that sent */


/*============================   ZW_NewController   ==========================
**
**    Start or stop the process of duplication a controller.
**    The state of this function are:
**
**    NEW_CTRL_STATE_SEND    - Send information to other controller
**    NEW_CTRL_STATE_RECEIVE - Receive information from another controller
**    NEW_CTRL_STATE_STOP    - Stop the controller replication process
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_NewController( /* RET Nothing */
  uint8_t bState,      /* IN  State */
  VOID_CALLBACKFUNC(completedFunc)(uint8_t, uint8_t, uint8_t*, uint8_t)); /* IN Callback function */


/*========================   TransferDoneCallback   =========================
**
**    Disable transfer state machine  and call application callback
**    function.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferDoneCallback( /* RET Nothing */
  uint8_t bStatus);


/*============================   SendTransferEnd  ===========================
**    Function description
**      Use this function to terminate replication.
**    Side effects:
**      Stops replication.
**--------------------------------------------------------------------------*/
void ZCB_SendTransferEnd(
  ZW_Void_Function_t Context,
  uint8_t state,
  TX_STATUS_TYPE *txStatusReport);


/*======================   TransferNodeInfoReceived   =======================
**
**    Transfer Node Info frame was received, save the information and send
**    a complete frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferNodeInfoReceived(  /*RET Nothing */
  uint8_t bCmdLength,         /* IN Command payload length */
  uint8_t *pCmd);             /* IN Pointer to node frame payload */


/*======================   TransferRangeInfoReceived   ======================
**
**    Transfer Range Info frame was received, save the information and send
**    a complete frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferRangeInfoReceived( /*RET Nothing */
  uint8_t bCmdLength,         /* IN Command payload length */
  uint8_t *pCmd);             /* IN Pointer to node frame payload */


/*=====================   TransferCmdCompleteReceived   ====================
**
**    Command complete frame was received, Send next information
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferCmdCompleteReceived(void); /* RET Nothing */


/*=============================== SendSUCID  ===================================
** Send SUC ID
**----------------------------------------------------------------------------*/
uint8_t
SendSUCID(
  uint8_t node,
  uint8_t txOption,
  const STransmitCallback* pTxCallback);


/*==========================   ReplicationInit   ===========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationInit(
  uint8_t bMode,
  void ( *completedFunc)(LEARN_INFO_T *)      /* IN Callback function */
);


/*=======================   StartReplicationReceiveTimer   ===================
**    Function description
**      Start/Restart Replication Receive timeout handling
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
StartReplicationReceiveTimer(void);

#endif /* _REPLICATION_H_ */
