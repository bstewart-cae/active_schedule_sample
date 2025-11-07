// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/** @file ZW_transport_service_wrapper.c
 * @brief Wrapper for the Transport Service module
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <stdbool.h>
#include "ZW_transport_api.h"
#include "ZW_transport.h"
#include <string.h>

/* This is a Transport Service style callback function without context
 * We cache it here while ZW_SendDataEx is working. */
static void (*ts_sendraw_stored_callback_func)(unsigned char status, void* user) = NULL;

/** Remove the context pointer from ZW_SendDataEx() call.
 * Transport Service has no need for it. (TODO: Refactor to use the context pointer) */
void TS_SendRaw_Cb_unwrapper(__attribute__((unused)) ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus)
{
  if(NULL != ts_sendraw_stored_callback_func)
  {
    ts_sendraw_stored_callback_func(txStatus, (void*)extendedTxStatus);
    ts_sendraw_stored_callback_func = NULL;
  }
}

/*
 * Called by Transport Service Module to send a frame to the radio.
 * Converts between the TS API and the Z-Wave protocol API.
 */
bool TS_SendRaw(node_id_t dst, uint8_t *buf, uint8_t buflen, uint8_t txopt, VOID_CALLBACKFUNC(cb)(unsigned char status, void* user))
{
  TRANSMIT_OPTIONS_TYPE ts_txo;
  memset((uint8_t*)&ts_txo, 0, sizeof(ts_txo));
  ts_txo.destNode = dst;
  ts_txo.txOptions = txopt;
  const STransmitCallback ts_sendraw_callback = { .pCallback = TS_SendRaw_Cb_unwrapper, .Context = NULL };
  ts_sendraw_stored_callback_func = cb;
  return (ZW_SendDataEx(buf, buflen, &ts_txo, &ts_sendraw_callback) == ZW_TX_IN_PROGRESS) ? true : false;
}
