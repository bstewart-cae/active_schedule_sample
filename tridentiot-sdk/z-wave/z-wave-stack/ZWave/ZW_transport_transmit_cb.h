// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_transmit_cb.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZW_TRANSPORT_TRANSMIT_CB_H_
#define ZW_TRANSPORT_TRANSMIT_CB_H_

#include <stdint.h>
#include <stdbool.h>
#include "ZW_transport_api.h"

typedef void(*ZW_Void_Function_t)(void);

typedef void(*ZW_SendData_Callback_t)(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus);

// Content considered public - use of bind/unbind/invoke methods are not mandatory (use of invoke is recommended)
typedef struct STransmitCallback
{
  ZW_SendData_Callback_t pCallback;

  ZW_Void_Function_t Context;  // A function pointer provided along with the callback method pointer
                               // to allow the callback receiver to identify
                               // the particular frame the callback is referring to.
} STransmitCallback;

bool ZW_TransmitCallbackIsBound(const STransmitCallback* pThis);

void ZW_TransmitCallbackBind(STransmitCallback* pThis, ZW_SendData_Callback_t pCallback, ZW_Void_Function_t Context);

void ZW_TransmitCallbackUnBind(STransmitCallback* pThis);

void ZW_TransmitCallbackInvoke(const STransmitCallback* pThis, uint8_t txStatus, TX_STATUS_TYPE* pextendedTxStatus);


// Special struct used by S2 security passing a pointer instead of a function pointer to the callback function
typedef struct STransmitCallbackPointer
{
  ZW_SendData_Callback_t pCallback;

  void * Context;   // A void pointer provided along with the callback method pointer
                    // to allow the callback receiver to identify
                    // the particular frame the callback is referring to.
} STransmitCallbackPointer;

#endif /* ZW_TRANSPORT_TRANSMIT_CB_H_ */
