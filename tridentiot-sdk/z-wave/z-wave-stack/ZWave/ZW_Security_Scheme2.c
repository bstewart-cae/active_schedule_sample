// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme2.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_lib_defines.h>

#include <ZW_libsec.h>
#include <platform.h>
#include <stdint.h>
#include <ZW_Security_Scheme2.h>
#include <ZW_ctimer.h>
#include <s2_inclusion.h>
#include <s2_keystore.h>
#include <zpal_entropy.h>
#include <ZW_transport.h>
#include "ZW_protocol_interface.h"
#include <ZW_keystore.h>
#include <ZW_s2_inclusion_glue.h>
#include <zpal_power_manager.h>
#include <ZW_node.h>

#ifdef ZW_CONTROLLER
#include <ZW_controller_network_info_storage.h>
#else
#include <ZW_slave_network_info_storage.h>
#endif

//#define DEBUGPRINT
#include <DebugPrint.h>


struct S2* s2_ctx = NULL;

#ifdef ZW_SECURITY_PROTOCOL
static STransmitCallback s2_send_callback;

static STransmitCallback s2_send_multi_callback;
static STransmitCallback cb_multi_save;

// Storage of the frame to be send, so that S2 does not require the package to remain
// available in the higher earlier levels of protocol and app.
static uint8_t aFrame[TX_BUFFER_SIZE];  // Array used for Tx storage, but RX_MAX also covers this.

zpal_pm_handle_t s2_power_lock;

static struct ctimer s2_timer;
/* This struct holds the extended tx status info until it can be delivered to the application.
 * It will be delivered when s2_protocol has finished. */


TX_STATUS_TYPE sExtendedTxStatusS2;

void ZCB_s2_evt_handler(zwave_event_t *evt); //TODO: Move function here and remove prototype


static void
s2_restore_spans_and_mpans(void)
{
/* Restore critical NodeID SPAN from Critical RAM into span_table - if valid... */

  StorageGetS2MpanTable((uint8_t*)&s2_ctx->mpan_table);
  StorageGetS2SpanTable((uint8_t*)&s2_ctx->span_table);
}


static void s2_save_spans_and_mpans(uint8_t reset)
{
  DPRINT("SAVE s\r\n");

  if (reset)
  {
    // If reset is true just force writes zeros in the nonces file
    memset((uint8_t *)&s2_ctx->mpan_table, 0, sizeof(s2_ctx->mpan_table));
    memset((uint8_t *)&s2_ctx->span_table, 0, sizeof(s2_ctx->span_table));
  }

  StorageSetS2MpanTable((uint8_t *)&s2_ctx->mpan_table);
  StorageSetS2SpanTable((uint8_t *)&s2_ctx->span_table);

  DPRINT("SAVE e\r\n");
}

void sec2_register_power_locks(void)
{
  s2_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
}

/**
 * Initialize Security 2 and return true (else false) if security 2 key exist (none zero).
 */
bool
sec2_init(void)
{
  S2_init_prng();

  uint32_t* pHomeIdUint32 = (uint32_t*)ZW_HomeIDGet();  // First cast to uint32 pointer to avoid
                                                      // GCC type-punned pointer error
  /* convert big endian homeid to native format needed by libs2 */
  s2_ctx = S2_init_ctx(UIP_HTONL(*pHomeIdUint32));
  s2_restore_spans_and_mpans();
  s2_inclusion_set_event_handler(ZCB_s2_evt_handler);
  zpal_pm_cancel(s2_power_lock);

  return (0 != (SECURITY_KEY_S2_MASK & keystore_get_cached_security_keys()));
}


void sec2_reset_nonces_tables(void)
{
  s2_save_spans_and_mpans(true);
}


void
sec2_PowerDownHandler(void)
{
  if (NULL !=s2_ctx)
  {
    s2_save_spans_and_mpans(false);
  }
}


/**
 * Stop S2 inclusion machine - if applicable
 */
void
sec2_inclusion_abort(void)
{
  s2_inclusion_abort(s2_ctx);
}


uint8_t
sec2_send_data(
  void* vdata,
  uint16_t len,
  TRANSMIT_OPTIONS_TYPE *txOptionsEx,
  const STransmitCallback* pCallback)
{
  uint8_t retval = 0;
  if (S2_is_send_data_busy(s2_ctx))
  {
    return retval;
  }

  //Safeguard against buffer overflow
  if (len > sizeof(aFrame))
  {
    return 0;
  }

  // Copy frame to local buffer
  memcpy(aFrame, vdata, len);

  s2_connection_t s2_con = {
    .l_node = txOptionsEx->bSrcNode,
    .r_node = txOptionsEx->destNode,
    .tx_options = txOptionsEx->txSecOptions,
    .zw_tx_options = txOptionsEx->txOptions,
    .class_id = txOptionsEx->securityKey - 1
  };

  s2_send_callback = *pCallback;

  retval = S2_send_data(s2_ctx, &s2_con, (uint8_t*)aFrame, len);
  if (0 != retval)
  {
    zpal_pm_stay_awake(s2_power_lock, 10000);
  }
  return retval;
}


/**
 * Emitted when the security engine is done.
 * Note that this is also emitted when the security layer sends a SECURE_COMMANDS_SUPPORTED_REPORT
 *
 * This will also deliver the stored  Extended Tx Status (RSSI, IMA info) to the application.
 * Extended Tx status is not supported by libs2. So we retrieve it from sExtendedTxStatusS2 here.
 */
void
S2_send_done_event(
  __attribute__((unused)) struct S2* ctxt,
  s2_tx_status_t status)
{
  zpal_pm_cancel(s2_power_lock);
  if(ZW_TransmitCallbackIsBound(&s2_send_multi_callback))
  {
    DPRINTF("m%02X", status);
    cb_multi_save = s2_send_multi_callback;
    ZW_TransmitCallbackUnBind(&s2_send_multi_callback);
    ZW_TransmitCallbackInvoke(&cb_multi_save, status, NULL);
  }
  else
  {
    DPRINTF("%02X", status);
    ZW_TransmitCallbackInvoke(&s2_send_callback, status, &sExtendedTxStatusS2);
    ZW_TransmitCallbackUnBind(&s2_send_callback);
  }
}


/**
 * Emitted when a messages has been received and decrypted
 */
void
S2_msg_received_event(
  __attribute__((unused)) struct S2* ctxt,
  s2_connection_t* src,
  uint8_t* buf,
  uint16_t len)
{
  RECEIVE_OPTIONS_TYPE rxOpt = {
  /* Frame header info */
    .rxStatus = src->zw_rx_status,
  /* Command sender Node ID */
    .sourceNode = src->r_node,
  /* Frame destination ID, only valid when frame is not Multicast*/
    .destNode = src->l_node,
  /* Average RSSI val in dBm  */
    .rxRSSIVal = src->zw_rx_RSSIval,
  /* Security scheme frame was received with. */
    .securityKey = src->class_id + 1 /* convert from class_id to security_key_t */
  };

  if(len < 2)
  {
    return;
  }
#ifdef ZW_BEAM_RX_WAKEUP
  if (0 != (src->rx_options & S2_RXOPTION_FOLLOWUP))
  {
    /* The received frame was a s2 multicast followup, inform Z-Wave protocol */
    ZW_FollowUpReceived();
  }
#endif

  ProtocolInterfacePassToAppSingleFrame(len, (ZW_APPLICATION_TX_BUFFER*)buf, &rxOpt);
}


static void
ZCB_S2_send_frame_callback(
  void* Context,
  uint8_t txStatus,
  TX_STATUS_TYPE * extendedTxStatus)
{
  DPRINTF("? %02X%02X", extendedTxStatus->TransmitTicks, txStatus);

  /* Save the extended tx status. It will be restored and delivered to application
   * after we have passed through libs2. */
  if(extendedTxStatus) {
    sExtendedTxStatusS2 = *extendedTxStatus;
  } else {
    // Clear variable
    memset(&sExtendedTxStatusS2, 0, sizeof(sExtendedTxStatusS2));
  }
  S2_send_frame_done_notify((struct S2*)Context, txStatus, (uint16_t)/*extendedTxStatus->wTransmitTicks*10*/500);
}


/** Must be implemented elsewhere maps to ZW_SendData or ZW_SendDataBridge note that ctxt is
 * passed as a handle. The ctxt MUST be provided when the \ref S2_send_frame_done_notify is called */
uint8_t
S2_send_frame(
  struct S2* ctxt,
  const s2_connection_t* conn,
  uint8_t* buf,
  uint16_t len)
{
  static TRANSMIT_OPTIONS_TYPE txo;

  memset((uint8_t*)&txo, 0, sizeof(txo));
  /*Swap source and destination*/
  txo.bSrcNode = conn->l_node;
  txo.destNode = conn->r_node;
  txo.txOptions = conn->zw_tx_options;
  txo.txOptions2 = TRANSMIT_OPTION_2_TRANSPORT_SERVICE;

  STransmitCallbackPointer TxCallback = { .pCallback = (ZW_SendData_Callback_t)ZCB_S2_send_frame_callback, .Context = (void *)ctxt };
  return (uint8_t)ZW_SendDataEx(buf, (uint8_t)len, &txo, (STransmitCallback *)&TxCallback);
}

/** Calls ZW_SendDataEx without callback
 */
uint8_t
S2_send_frame_no_cb(
  __attribute__((unused)) struct S2* ctxt,
  const s2_connection_t* conn,
  uint8_t* buf,
  uint16_t len)
{
  static TRANSMIT_OPTIONS_TYPE txo;

  memset((uint8_t*)&txo, 0, sizeof(txo));
  /*Swap source and destination*/
  txo.bSrcNode = conn->l_node;
  txo.destNode = conn->r_node;
  txo.txOptions = conn->zw_tx_options;
  txo.txOptions2 = TRANSMIT_OPTION_2_TRANSPORT_SERVICE;

  STransmitCallbackPointer TxCallback = { .pCallback = NULL, .Context = 0 };
  return (uint8_t)ZW_SendDataEx(buf, (uint8_t)len, &txo, (STransmitCallback *)&TxCallback);
}

uint8_t
sec2_send_data_multi(
  void* vdata,
  uint16_t len,
  TRANSMIT_MULTI_OPTIONS_TYPE *txOptionsMultiEx,
  const STransmitCallback* pCallback)
{
  if (S2_is_send_data_multicast_busy(s2_ctx))
  {
    return 0;
  }

  //Safeguard against buffer overflow
  if (len > sizeof(aFrame))
  {
    return 0;
  }

  // Copy frame to local buffer
  memcpy(aFrame, vdata, len);

  s2_connection_t s2_con = {
    .l_node = txOptionsMultiEx->bSrcNode,
  /* Multicast groupID */
    .r_node = txOptionsMultiEx->groupID,
    .zw_tx_options = txOptionsMultiEx->txOptions,
    .class_id = txOptionsMultiEx->securityKey - 1,
    .tx_options = 0
  };

  s2_send_multi_callback = *pCallback;
  return S2_send_data_multicast(s2_ctx, &s2_con, (uint8_t*)aFrame, len);
}


/** Must be implemented elsewhere maps to ZW_SendDataMulti that ctxt is
 * passed as a handle. The ctxt MUST be provided when the \ref S2_send_frame_done_notify is called
 * the conn->r_node is the multicast group id
 */
uint8_t
S2_send_frame_multi(
  struct S2* ctxt,
  s2_connection_t* conn,
  uint8_t* buf,
  uint16_t len)
{
  static TRANSMIT_OPTIONS_TYPE s2_sfm_txopt;

  memset((uint8_t*)&s2_sfm_txopt, 0, sizeof(s2_sfm_txopt));
  /*Swap source and destination*/
  s2_sfm_txopt.bSrcNode = conn->l_node;
  s2_sfm_txopt.destNode = 0xFF;
  s2_sfm_txopt.txOptions = conn->zw_tx_options;

  STransmitCallbackPointer TxCallback = { .pCallback = (ZW_SendData_Callback_t)ZCB_S2_send_frame_callback, .Context = (void *)ctxt };
  return (uint8_t)ZW_SendDataEx(buf, (uint8_t)len, &s2_sfm_txopt, (STransmitCallback *)&TxCallback);
}


static void
ZCB_timeout2(
  void* ctxt)
{
  S2_timeout_notify((struct S2*)ctxt);
}


/**
 * Must be implemented elsewhere maps to ZW_TimerStart. Note that this must start the same timer every time.
 * Ie. two calls to this function must reset the timer to a new value. On timout \ref S2_timeout_event must be called.
 *
 */
void
S2_set_timeout(
  struct S2* ctxt,
  uint32_t interval)
{
  DPRINTF("%08X", interval);
  ctimer_set(&s2_timer,interval, ZCB_timeout2, (void *)ctxt);
}

void S2_stop_timeout(__attribute__((unused)) struct S2* ctxt)
{
  ctimer_stop(&s2_timer);
}


/**
 * Get a number of bytes from a hardware random number generator. len must be a multiple of 2
 */
void
S2_get_hw_random(
  uint8_t *buf,
  uint8_t len)
{
  zpal_get_random_data(buf, len);
}


void /*RET Nothing                  */
Security2CommandHandler(
  uint8_t *pCmd,        /* IN  Frame payload pointer */
  uint8_t cmdLength,    /* IN  Payload length        */
  RECEIVE_OPTIONS_TYPE *rxopt)
{
  s2_connection_t conn = {
    .r_node = rxopt->sourceNode,
    .l_node = g_nodeID, //rxopt->destNode;
    .rx_options = ((RECEIVE_STATUS_TYPE_MULTI|RECEIVE_STATUS_TYPE_BROAD) & rxopt->rxStatus) ? S2_RXOPTION_MULTICAST : 0,
    .zw_rx_status = rxopt->rxStatus,
    .zw_tx_options = ((rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER) ? TRANSMIT_OPTION_LOW_POWER : 0) | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE,
    .zw_rx_RSSIval = rxopt->rxRSSIVal,
    .tx_options = 0
  };

  if (pCmd[0] != COMMAND_CLASS_SECURITY_2)
  {
    return;
  }

  if ((KEX_SET == pCmd[SECURITY_2_COMMAND_POS]) &&
	    (0 == (pCmd[SECURITY_2_KEX_SET_KEYS_POS] & (SECURITY_KEY_S2_AUTHENTICATED_BIT | SECURITY_KEY_S2_ACCESS_BIT))))
  {
    // dynamic keypair will only be generated if secure bootstrapping was not initiated through SmartStart
	  keystore_dynamic_keypair_generate();
  }
  S2_application_command_handler(s2_ctx, &conn, (uint8_t*) pCmd, (uint16_t) cmdLength);

  /* Some but not all frames should keep device awake.
   * On secure network, all frames are encrypted and match SECURITY_2_MESSAGE_ENCAPSULATION
   * We are only interested in CC Security frames, like SECURITY_2_COMMANDS_SUPPORTED_GET
   * If S2 is busy, then the encapsulated frame is from Security 2 CC.
   * Even if the message cannot be decrypted (NONCE_GET, NONCE_REPORT), the wake up timer MUST be restarted. (RT:00.11.0021.2)
   * */
  if (((SECURITY_2_MESSAGE_ENCAPSULATION == pCmd[SECURITY_2_COMMAND_POS]) ||
      (SECURITY_2_NONCE_GET == pCmd[SECURITY_2_COMMAND_POS]) ||
      (SECURITY_2_NONCE_REPORT == pCmd[SECURITY_2_COMMAND_POS])) ||
      s2_busy() )
  {
    ProtocolInterfacePassToAppStayAwake();
  }
}


/**
* Return true if the inclusion fsm is busy
*/
uint8_t
s2_busy(void)
{
  return S2_is_busy(s2_ctx);
}

/**
 * Called when an S2 Resynchronization event happens.
 * It is entirely optional to do anything when this is called
 * (but libs2 requires it to be implemented).
 */
void S2_resynchronization_event(
    __attribute__((unused)) node_t remote_node,
    __attribute__((unused)) sos_event_reason_t reason,
    __attribute__((unused)) uint8_t seqno,
    __attribute__((unused)) node_t local_node)
{
  // Stub function. See function doxygen for explanation.
}
#endif /* #ifdef ZW_SECURITY_PROTOCOL */
