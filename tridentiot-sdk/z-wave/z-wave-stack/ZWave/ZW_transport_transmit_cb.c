// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_transmit_cb.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_transport_transmit_cb.h>
//#define DEBUGPRINT
#include "DebugPrint.h"
#include "Assert.h"

bool ZW_TransmitCallbackIsBound(const STransmitCallback* pThis)
{
  return pThis->pCallback;
}

void ZW_TransmitCallbackBind(STransmitCallback* pThis, ZW_SendData_Callback_t pCallback, ZW_Void_Function_t Context)
{
  pThis->pCallback = pCallback;
  pThis->Context = Context;
}

void ZW_TransmitCallbackUnBind(STransmitCallback* pThis)
{
  pThis->pCallback = 0;
  pThis->Context = 0;
}

void ZW_TransmitCallbackInvoke(const STransmitCallback* pThis, uint8_t txStatus, TX_STATUS_TYPE* pextendedTxStatus)
{
  if (pThis->pCallback)
  {
    DPRINTF("Callback at %08X\r\n", pThis->pCallback);
    pThis->pCallback(pThis->Context, txStatus, pextendedTxStatus);
  }
}
